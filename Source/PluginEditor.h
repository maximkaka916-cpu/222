#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "PluginProcessor.h"

/** Кавайные ручки с мордочками и пилюлеобразная кнопка. */
class EvvscyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EvvscyLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class EvvscyEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit EvvscyEditor (EvvscySupressorProcessor&);
    ~EvvscyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawMascot (juce::Graphics&, juce::Rectangle<float> area);

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    EvvscySupressorProcessor& proc;
    EvvscyLookAndFeel laf;

    std::array<Knob, 9> knobs;
    juce::ToggleButton deltaButton { "Delta" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> deltaAttachment;

    float shownReduction = 0.0f;
    float phase = 0.0f;
    int frame = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EvvscyEditor)
};
