#pragma once

#include <JuceHeader.h>
#include "LA2ADSP.h"

class ALX_LA2A_AudioProcessor : public juce::AudioProcessor
{
public:
    ALX_LA2A_AudioProcessor();
    ~ALX_LA2A_AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ALX-LA-2A"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Текущая gain reduction для отрисовки VU-метра в редакторе (потокобезопасно)
    std::atomic<float> currentGRdb { 0.0f };
    std::atomic<float> currentOutLevelDb { -60.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    alx::OptoDetector detector;
    alx::TubeStage tubeL, tubeR;

    float prevOutL = 0.0f, prevOutR = 0.0f;

    juce::SmoothedValue<float> smoothedPeakReduction;
    juce::SmoothedValue<float> smoothedMakeupGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ALX_LA2A_AudioProcessor)
};
