#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

EvvscySupressorProcessor::EvvscySupressorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

APVTS::ParameterLayout EvvscySupressorProcessor::createLayout()
{
    using juce::AudioParameterFloat;
    using juce::AudioParameterBool;
    using juce::NormalisableRange;
    using juce::ParameterID;

    APVTS::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "depth", 1 }, "Depth",
                NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "threshold", 1 }, "Threshold (dB)",
                NormalisableRange<float> (0.0f, 15.0f, 0.1f), 4.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "width", 1 }, "Width (oct)",
                NormalisableRange<float> (0.25f, 3.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack (ms)",
                NormalisableRange<float> (1.0f, 200.0f, 0.1f, 0.5f), 10.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release (ms)",
                NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.5f), 100.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "maxred", 1 }, "Max Reduction (dB)",
                NormalisableRange<float> (0.0f, 24.0f, 0.1f), 12.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "lowfreq", 1 }, "Low Freq (Hz)",
                NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 100.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "highfreq", 1 }, "High Freq (Hz)",
                NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 12000.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix (%)",
                NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "delta", 1 }, "Delta (listen to removed)", false));

    return layout;
}

void EvvscySupressorProcessor::prepareToPlay (double sampleRate, int)
{
    suppressor.prepare (sampleRate, getTotalNumInputChannels());
    setLatencySamples (ResonanceSuppressor::fftSize);
}

bool EvvscySupressorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == layouts.getMainInputChannelSet();
}

void EvvscySupressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    ResonanceSuppressor::Params p;
    p.depth          = get ("depth") / 100.0f;
    p.thresholdDb    = get ("threshold");
    p.widthOct       = get ("width");
    p.attackMs       = get ("attack");
    p.releaseMs      = get ("release");
    p.maxReductionDb = get ("maxred");
    p.lowHz          = std::min (get ("lowfreq"), get ("highfreq"));
    p.highHz         = std::max (get ("lowfreq"), get ("highfreq"));

    suppressor.setParams (p);
    suppressor.process (buffer, get ("mix") / 100.0f, get ("delta") > 0.5f);
}

juce::AudioProcessorEditor* EvvscySupressorProcessor::createEditor()
{
    return new EvvscyEditor (*this);
}

void EvvscySupressorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void EvvscySupressorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EvvscySupressorProcessor();
}
