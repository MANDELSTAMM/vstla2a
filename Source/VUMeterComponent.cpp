#include "VUMeterComponent.h"

VUMeterComponent::VUMeterComponent()
{
    startTimerHz (60);
}

VUMeterComponent::~VUMeterComponent()
{
    stopTimer();
}

void VUMeterComponent::timerCallback()
{
    const float smoothing = 0.18f; // лёгкая механическая инерция стрелки
    smoothedGR        += (targetGR - smoothedGR) * smoothing;
    smoothedOutLevel   += (targetOutLevel - smoothedOutLevel) * smoothing;

    const float targetAngle = computeNeedleNormalised();
    needleAngle += (targetAngle - needleAngle) * 0.35f;

    repaint();
}

float VUMeterComponent::computeNeedleNormalised() const
{
    if (meterMode == 0)
    {
        // GR: 0 дБ редукции -> +40°, ~20дБ редукции -> -40°
        const float n = juce::jlimit (0.0f, 1.0f, smoothedGR / 20.0f);
        return juce::jmap (n, 0.0f, 1.0f, 40.0f, -40.0f);
    }
    else
    {
        const float n = juce::jlimit (0.0f, 1.0f, (smoothedOutLevel + 20.0f) / 26.0f);
        return juce::jmap (n, 0.0f, 1.0f, -40.0f, 40.0f);
    }
}

void VUMeterComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // --- Хромированная рамка ---
    auto bezel = bounds.reduced (2.0f);
    juce::ColourGradient bezelGrad (juce::Colour (0xffd8d8dc), bezel.getTopLeft(),
                                     juce::Colour (0xff6a6a70), bezel.getBottomRight(), false);
    g.setGradientFill (bezelGrad);
    g.fillRoundedRectangle (bezel, 10.0f);

    auto face = bezel.reduced (7.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (face, 6.0f);

    face = face.reduced (3.0f);

    // --- Цвет циферблата: тёплый кремовый, чуть темнее по краям ---
    juce::ColourGradient faceGrad (juce::Colour (0xfff3ead2), face.getCentreX(), face.getY(),
                                    juce::Colour (0xffd9caa0), face.getCentreX(), face.getBottom(), false);
    g.setGradientFill (faceGrad);
    g.fillRoundedRectangle (face, 5.0f);

    const float pivotX = face.getCentreX();
    const float pivotY = face.getBottom() + face.getHeight() * 0.30f;
    const float radius = face.getHeight() * 1.05f;

    // --- Шкала ---
    g.setColour (juce::Colour (0xff2a2210));
    juce::Path scaleArc;
    const float arcR1 = radius * 0.86f;

    auto degToRad = [] (float d) { return juce::MathConstants<float>::pi * d / 180.0f; };

    for (int i = -40; i <= 40; i += 10)
    {
        const float a = degToRad ((float) i - 90.0f);
        const float x1 = pivotX + std::cos (a) * arcR1;
        const float y1 = pivotY + std::sin (a) * arcR1;
        const float x2 = pivotX + std::cos (a) * (arcR1 - 7.0f);
        const float y2 = pivotY + std::sin (a) * (arcR1 - 7.0f);
        g.drawLine (x1, y1, x2, y2, 1.6f);
    }

    // Красная зона (классическая зона "перегруза" VU справа)
    {
        juce::Path redZone;
        const float aStart = degToRad (24.0f - 90.0f);
        const float aEnd   = degToRad (40.0f - 90.0f);
        redZone.addPieSegment (pivotX - arcR1, pivotY - arcR1, arcR1 * 2.0f, arcR1 * 2.0f,
                                aStart, aEnd, 0.88f);
        g.setColour (juce::Colour (0xffb23a2e));
        g.fillPath (redZone);
    }

    g.setColour (juce::Colour (0xff2a2210));
    juce::Font numFont (juce::FontOptions (face.getHeight() * 0.16f, juce::Font::bold));
    g.setFont (numFont);

    struct Tick { int deg; const char* label; };
    const Tick ticks[] = { { -40, "-20" }, { -20, "-10" }, { 0, "0" }, { 20, "+5" }, { 40, "+8" } };

    for (auto& t : ticks)
    {
        const float a = degToRad ((float) t.deg - 90.0f);
        const float lx = pivotX + std::cos (a) * (arcR1 - 22.0f);
        const float ly = pivotY + std::sin (a) * (arcR1 - 22.0f);
        g.drawText (t.label, juce::Rectangle<float> (lx - 14.0f, ly - 8.0f, 28.0f, 16.0f),
                    juce::Justification::centred, false);
    }

    // Подпись VU
    juce::Font vuFont (juce::FontOptions (face.getHeight() * 0.22f, juce::Font::bold));
    g.setFont (vuFont);
    g.drawText ("VU", juce::Rectangle<float> (pivotX - 30.0f, pivotY - radius * 0.62f, 60.0f, 26.0f),
                juce::Justification::centred);

    juce::Font brandFont (juce::FontOptions (getHeight() * 0.085f, juce::Font::plain));
    g.setFont (brandFont);
    g.setColour (juce::Colour (0xff4a3c1e));
    g.drawText ("ALX-LA-2A", juce::Rectangle<float> (pivotX - 60.0f, pivotY - radius * 0.30f, 120.0f, 18.0f),
                juce::Justification::centred);

    // --- Стрелка ---
    {
        const float a = degToRad (needleAngle - 90.0f);
        const float tipX = pivotX + std::cos (a) * (radius * 0.92f);
        const float tipY = pivotY + std::sin (a) * (radius * 0.92f);

        juce::Path needle;
        needle.startNewSubPath (pivotX, pivotY);
        needle.lineTo (tipX, tipY);

        g.setColour (juce::Colours::black);
        g.strokePath (needle, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (juce::Colour (0xffb23a2e));
        g.fillEllipse (pivotX - 5.0f, pivotY - 5.0f, 10.0f, 10.0f);
    }

    // --- Стеклянный блик ---
    juce::ColourGradient glass (juce::Colours::white.withAlpha (0.18f), face.getX(), face.getY(),
                                 juce::Colours::white.withAlpha (0.0f), face.getX(), face.getY() + face.getHeight() * 0.6f, false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (face, 5.0f);
}
