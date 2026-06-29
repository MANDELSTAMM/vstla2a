#pragma once
#include <JuceHeader.h>

/** Винтажный VU-метр со стрелкой и подсветкой циферблата, как на LA-2A.
    Режимы: GR (gain reduction, стрелка качается влево по мере компрессии)
    и +4 / +8 (выходной уровень относительно опорной точки). */
class VUMeterComponent : public juce::Component, private juce::Timer
{
public:
    VUMeterComponent();
    ~VUMeterComponent() override;

    void paint (juce::Graphics&) override;

    void setGRdb (float grDb)      { targetGR = grDb; }
    void setOutLevelDb (float lvl) { targetOutLevel = lvl; }
    void setMeterMode (int mode)   { meterMode = mode; } // 0=GR,1=+4,2=+8

private:
    void timerCallback() override;
    float computeNeedleNormalised() const;

    float targetGR = 0.0f, smoothedGR = 0.0f;
    float targetOutLevel = -60.0f, smoothedOutLevel = -60.0f;
    float needleAngle = 0.0f; // текущий угол для лёгкой механической инерции
    int meterMode = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUMeterComponent)
};
