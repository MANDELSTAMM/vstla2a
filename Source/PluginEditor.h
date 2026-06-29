#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "VUMeterComponent.h"

//==============================================================================
/** Кастомный внешний вид: бакелитовые ручки, рокерные переключатели,
    металлическая обводка - всё в духе винтажных студийных приборов. */
class ALXLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ALXLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                            float sliderPosProportional, float rotaryStartAngle,
                            float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                        int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
};

//==============================================================================
class ALX_LA2A_AudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ALX_LA2A_AudioProcessorEditor (ALX_LA2A_AudioProcessor&);
    ~ALX_LA2A_AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawScrew (juce::Graphics& g, float cx, float cy, float r);

    ALX_LA2A_AudioProcessor& processorRef;
    ALXLookAndFeel lookAndFeel;

    VUMeterComponent vuMeter;

    juce::Slider peakReductionKnob, gainKnob;
    juce::Label  peakReductionLabel, gainLabel;

    juce::ToggleButton powerSwitch, limitModeSwitch;
    juce::Label powerLabel, modeLabelOff, modeLabelOn, powerLabelOff, powerLabelOn;

    juce::ComboBox meterCombo;
    juce::Label meterLabel;

    juce::Label titleLabel, subtitleLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakReductionAttachment, gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment, limitModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> meterAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ALX_LA2A_AudioProcessorEditor)
};
