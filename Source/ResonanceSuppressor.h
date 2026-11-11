#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <atomic>

/**
    STFT-based resonance suppressor.

    Для каждого кадра (FFT 2048, перекрытие 75%, окно Хэннинга):
      1. считаем спектр мощности в дБ (для стерео берём максимум по каналам - "linked");
      2. строим сглаженную огибающую спектра (среднее в дБ в окне шириной W октав);
      3. всё, что выступает над огибающей больше порога, считается резонансом;
      4. для каждого бина считаем ослабление, сглаживаем по частоте и во времени
         (attack / release) и применяем к спектру;
      5. обратное FFT + overlap-add.

    Латентность = fftSize сэмплов (сообщается хосту через setLatencySamples).
*/
class ResonanceSuppressor
{
public:
    static constexpr int order       = 11;
    static constexpr int fftSize     = 1 << order;   // 2048
    static constexpr int hopSize     = fftSize / 4;  // 75% overlap
    static constexpr int numBins     = fftSize / 2 + 1;
    static constexpr int maxChannels = 2;

    struct Params
    {
        float depth          = 0.5f;    // 0..1  - насколько сильно давить превышение
        float thresholdDb    = 4.0f;    // дБ над огибающей, с которых начинается подавление
        float widthOct       = 1.0f;    // ширина окна сглаживания огибающей (октавы)
        float attackMs       = 10.0f;
        float releaseMs      = 100.0f;
        float maxReductionDb = 12.0f;
        float lowHz          = 100.0f;  // рабочий диапазон
        float highHz         = 12000.0f;
    };

    ResonanceSuppressor() : fft (order)
    {
        for (int i = 0; i < fftSize; ++i)   // периодический Хэннинг (COLA при hop = N/4)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) fftSize);

        reset();
    }

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        numCh = juce::jlimit (1, maxChannels, numChannels);
        lastWidth = -1.0f;
        reset();
        updateDerived();
    }

    /** Пиковое ослабление (дБ) - для индикатора в интерфейсе. */
    float getPeakReductionDb() const noexcept { return peakReduction.load(); }

    void reset()
    {
        for (auto& c : channels)
        {
            c.in.fill (0.0f);
            c.out.fill (0.0f);
            c.pos = 0;
        }
        grDb.fill (0.0f);
        hopCounter = 0;
    }

    void setParams (const Params& p)
    {
        params = p;
        updateDerived();
    }

    /** Обработка на месте. mix: 0..1, delta: слушать только то, что вырезано. */
    void process (juce::AudioBuffer<float>& buffer, float mix, bool delta)
    {
        const int numSamples = buffer.getNumSamples();
        const int chans = std::min (buffer.getNumChannels(), numCh);

        for (int i = 0; i < numSamples; ++i)
        {
            for (int c = 0; c < chans; ++c)
            {
                auto& s = channels[(size_t) c];
                float* data = buffer.getWritePointer (c);

                const float x   = data[i];
                const float dry = s.in[(size_t) s.pos];    // x[t - N] - задержанный сухой сигнал
                s.in[(size_t) s.pos] = x;

                const float wet = s.out[(size_t) s.pos];
                s.out[(size_t) s.pos] = 0.0f;

                s.pos = (s.pos + 1) & (fftSize - 1);

                data[i] = delta ? (dry - wet) : (dry + mix * (wet - dry));
            }

            if (++hopCounter >= hopSize)
            {
                hopCounter = 0;
                processFrame (chans);
            }
        }
    }

private:
    struct Channel
    {
        std::array<float, fftSize> in {}, out {};
        int pos = 0;
    };

    void updateDerived()
    {
        const double frameRate = sampleRate / (double) hopSize;
        coefAttack  = (float) std::exp (-1.0 / (std::max (0.1f, params.attackMs)  * 0.001 * frameRate));
        coefRelease = (float) std::exp (-1.0 / (std::max (0.1f, params.releaseMs) * 0.001 * frameRate));

        const float binHz = (float) (sampleRate / (double) fftSize);
        lowBin  = juce::jlimit (1, numBins - 2, (int) std::round (params.lowHz  / binHz));
        highBin = juce::jlimit (lowBin, numBins - 2, (int) std::round (params.highHz / binHz));

        if (params.widthOct != lastWidth)
        {
            lastWidth = params.widthOct;
            const float ratio = std::pow (2.0f, params.widthOct * 0.5f);

            for (int k = 0; k < numBins; ++k)
            {
                int lo = (int) ((float) k / ratio);
                int hi = (int) std::ceil ((float) k * ratio);
                lo = std::min (lo, k - 3);          // минимум +-3 бина
                hi = std::max (hi, k + 3);
                bandLo[(size_t) k] = std::max (0, lo);
                bandHi[(size_t) k] = std::min (numBins - 1, hi);
            }
        }
    }

    void processFrame (int chans)
    {
        // 1. окно + FFT
        for (int c = 0; c < chans; ++c)
        {
            auto& s = channels[(size_t) c];
            float* d = spec[(size_t) c].data();

            for (int i = 0; i < fftSize; ++i)
                d[i] = s.in[(size_t) ((s.pos + i) & (fftSize - 1))] * window[(size_t) i];

            std::fill (d + fftSize, d + 2 * fftSize, 0.0f);
            fft.performRealOnlyForwardTransform (d, true);
        }

        // 2. спектр мощности в дБ (максимум по каналам) + префиксные суммы
        prefix[0] = 0.0f;
        for (int k = 0; k < numBins; ++k)
        {
            float power = 0.0f;
            for (int c = 0; c < chans; ++c)
            {
                const float* d = spec[(size_t) c].data();
                power = std::max (power, d[2 * k] * d[2 * k] + d[2 * k + 1] * d[2 * k + 1]);
            }

            magDb[(size_t) k] = 10.0f * std::log10 (power + 1.0e-12f);
            prefix[(size_t) k + 1] = prefix[(size_t) k] + magDb[(size_t) k];
        }

        // 3. целевое ослабление
        for (int k = 0; k < numBins; ++k)
        {
            float red = 0.0f;

            if (k >= lowBin && k <= highBin)
            {
                const int lo = bandLo[(size_t) k], hi = bandHi[(size_t) k];
                const float avg = (prefix[(size_t) hi + 1] - prefix[(size_t) lo]) / (float) (hi - lo + 1);
                const float excess = magDb[(size_t) k] - avg - params.thresholdDb;

                if (excess > 0.0f)
                    red = std::min (excess * params.depth, params.maxReductionDb);
            }

            target[(size_t) k] = red;
        }

        // 4. сглаживание по частоте и по времени -> линейный gain
        float peak = 0.0f;
        for (int k = 0; k < numBins; ++k)
        {
            const float l = target[(size_t) std::max (k - 1, 0)];
            const float r = target[(size_t) std::min (k + 1, numBins - 1)];
            const float t = 0.25f * l + 0.5f * target[(size_t) k] + 0.25f * r;

            const float coef = t > grDb[(size_t) k] ? coefAttack : coefRelease;
            grDb[(size_t) k] = coef * grDb[(size_t) k] + (1.0f - coef) * t;

            gain[(size_t) k] = juce::Decibels::decibelsToGain (-grDb[(size_t) k]);
            peak = std::max (peak, grDb[(size_t) k]);
        }
        peakReduction.store (peak);

        // 5. применяем gain, IFFT, overlap-add
        constexpr float norm = 1.0f / 1.5f;   // сумма w^2 при hop = N/4 равна 1.5

        for (int c = 0; c < chans; ++c)
        {
            auto& s = channels[(size_t) c];
            float* d = spec[(size_t) c].data();

            for (int k = 0; k < numBins; ++k)
            {
                d[2 * k]     *= gain[(size_t) k];
                d[2 * k + 1] *= gain[(size_t) k];
            }

            fft.performRealOnlyInverseTransform (d);

            for (int i = 0; i < fftSize; ++i)
                s.out[(size_t) ((s.pos + i) & (fftSize - 1))] += d[i] * window[(size_t) i] * norm;
        }
    }

    juce::dsp::FFT fft;
    std::atomic<float> peakReduction { 0.0f };
    Params params;
    double sampleRate = 44100.0;
    int numCh = 2;
    int hopCounter = 0;
    int lowBin = 1, highBin = numBins - 2;
    float coefAttack = 0.9f, coefRelease = 0.99f, lastWidth = -1.0f;

    std::array<Channel, maxChannels> channels;
    std::array<std::array<float, 2 * fftSize>, maxChannels> spec {};
    std::array<float, fftSize> window {};
    std::array<float, numBins> magDb {}, target {}, grDb {}, gain {};
    std::array<float, numBins + 1> prefix {};
    std::array<int, numBins> bandLo {}, bandHi {};
};
