#include "PluginProcessor.h"
#include "PluginEditor.h"

ALX_LA2A_AudioProcessor::ALX_LA2A_AudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

ALX_LA2A_AudioProcessor::~ALX_LA2A_AudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout ALX_LA2A_AudioProcessor::createParameterLayout()
{
    using Param  = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;
    using Bool   = juce::AudioParameterBool;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<Param> (
        juce::ParameterID { "peakReduction", 1 }, "Peak Reduction",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<Param> (
        juce::ParameterID { "gain", 1 }, "Gain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back (std::make_unique<Bool> (
        juce::ParameterID { "limitMode", 1 }, "Limit Mode", false));

    params.push_back (std::make_unique<Bool> (
        juce::ParameterID { "power", 1 }, "Power", true));

    params.push_back (std::make_unique<Choice> (
        juce::ParameterID { "meter", 1 }, "Meter",
        juce::StringArray { "GR", "+4", "+8" }, 0));

    return { params.begin(), params.end() };
}

void ALX_LA2A_AudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    detector.prepare (sampleRate);
    prevOutL = prevOutR = 0.0f;

    smoothedPeakReduction.reset (sampleRate, 0.02);
    smoothedMakeupGain.reset (sampleRate, 0.02);

    auto* pr = apvts.getRawParameterValue ("peakReduction");
    auto* gn = apvts.getRawParameterValue ("gain");
    smoothedPeakReduction.setCurrentAndTargetValue (pr->load() / 100.0f);
    smoothedMakeupGain.setCurrentAndTargetValue (gn->load());
}

void ALX_LA2A_AudioProcessor::releaseResources() {}

bool ALX_LA2A_AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto set = layouts.getMainOutputChannelSet();
    return set == juce::AudioChannelSet::mono() || set == juce::AudioChannelSet::stereo();
}

void ALX_LA2A_AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    const bool power = apvts.getRawParameterValue ("power")->load() > 0.5f;
    const int  mode  = apvts.getRawParameterValue ("limitMode")->load() > 0.5f ? 1 : 0;

    smoothedPeakReduction.setTargetValue (apvts.getRawParameterValue ("peakReduction")->load() / 100.0f);
    smoothedMakeupGain.setTargetValue (apvts.getRawParameterValue ("gain")->load());

    if (! power)
    {
        // Полный честный bypass
        currentGRdb.store (0.0f);
        return;
    }

    auto* left  = buffer.getWritePointer (0);
    auto* right = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    float lastGRforMeter = 0.0f;
    float lastOutPeak = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        const float peakReduction = smoothedPeakReduction.getNextValue();
        const float gainKnob      = smoothedMakeupGain.getNextValue();
        const float makeupDb      = juce::jmap (gainKnob, 0.0f, 100.0f, -10.0f, 40.0f);
        const float makeupLin     = alx::dbToAmp (makeupDb);

        const float inL = left[n];
        const float inR = right != nullptr ? right[n] : inL;

        // Стерео-линкованная детекция: общий детектор слушает оба канала,
        // чтобы компрессия не "гуляла" по стереобазе.
        const float detLevel = std::max (std::abs (prevOutL), std::abs (prevOutR));
        const float grDb = detector.process (detLevel, peakReduction, mode);
        const float gainLin = alx::dbToAmp (-grDb);

        float preL = inL * gainLin;
        float preR = inR * gainLin;

        // Запоминаем pre-makeup сигнал для следующего шага детектора (feedback-топология)
        prevOutL = preL;
        prevOutR = preR;

        // Ламповый выходной каскад + makeup gain
        float outL = tubeL.process (preL) * makeupLin;
        float outR = tubeR.process (preR) * makeupLin;

        left[n] = outL;
        if (right != nullptr)
            right[n] = outR;

        lastGRforMeter = grDb;
        lastOutPeak = std::max (lastOutPeak, std::max (std::abs (outL), std::abs (outR)));
    }

    currentGRdb.store (lastGRforMeter);
    currentOutLevelDb.store (alx::ampToDb (lastOutPeak));
}

juce::AudioProcessorEditor* ALX_LA2A_AudioProcessor::createEditor()
{
    return new ALX_LA2A_AudioProcessorEditor (*this);
}

void ALX_LA2A_AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ALX_LA2A_AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ALX_LA2A_AudioProcessor();
}
