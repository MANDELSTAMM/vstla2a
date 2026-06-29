#include "PluginEditor.h"

//==============================================================================
ALXLookAndFeel::ALXLookAndFeel()
{
    setColour (juce::Slider::thumbColourId, juce::Colours::white);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a1a1a));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffd8d0b8));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff444444));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1a1a1a));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffd8d0b8));
}

void ALXLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Металлическая юбка
    juce::ColourGradient skirtGrad (juce::Colour (0xffe2e2e6), centre.x, centre.y - radius,
                                     juce::Colour (0xff5e5e64), centre.x, centre.y + radius, false);
    g.setGradientFill (skirtGrad);
    g.fillEllipse (bounds);

    g.setColour (juce::Colour (0xff202020));
    g.drawEllipse (bounds, 1.0f);

    // Тело ручки (бакелит)
    auto knobBounds = bounds.reduced (radius * 0.16f);
    juce::ColourGradient knobGrad (juce::Colour (0xff3a3a3a), knobBounds.getCentreX(), knobBounds.getY(),
                                    juce::Colour (0xff0a0a0a), knobBounds.getCentreX(), knobBounds.getBottom(), false);
    g.setGradientFill (knobGrad);
    g.fillEllipse (knobBounds);

    // Насечки рифления по краю
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    const int numNotches = 28;
    for (int i = 0; i < numNotches; ++i)
    {
        const float a = (float) i * (juce::MathConstants<float>::twoPi / (float) numNotches);
        const float r1 = radius * 0.86f, r2 = radius * 0.98f;
        juce::Line<float> notch (centre.x + std::cos (a) * r1, centre.y + std::sin (a) * r1,
                                  centre.x + std::cos (a) * r2, centre.y + std::sin (a) * r2);
        g.drawLine (notch, 1.0f);
    }

    // Указатель
    juce::Path pointer;
    const float pw = radius * 0.09f;
    pointer.addRoundedRectangle (-pw * 0.5f, -radius * 0.82f, pw, radius * 0.55f, pw * 0.4f);
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));

    // Блик
    auto highlight = knobBounds.reduced (knobBounds.getWidth() * 0.32f)
                          .translated (-knobBounds.getWidth() * 0.10f, -knobBounds.getHeight() * 0.14f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillEllipse (highlight);
}

void ALXLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                        bool /*highlighted*/, bool /*down*/)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
    const float h = bounds.getHeight();

    g.setColour (juce::Colour (0xff0c0c0c));
    g.fillRoundedRectangle (bounds, h * 0.5f);
    g.setColour (juce::Colour (0xff3a3a3a));
    g.drawRoundedRectangle (bounds, h * 0.5f, 1.2f);

    const bool on = button.getToggleState();
    const float knobDia = h - 5.0f;
    const float knobX = on ? bounds.getRight() - knobDia - 2.5f : bounds.getX() + 2.5f;
    juce::Rectangle<float> knobRect (knobX, bounds.getY() + 2.5f, knobDia, knobDia);

    if (on)
    {
        g.setColour (juce::Colour (0xffb23a2e).withAlpha (0.55f));
        g.fillRoundedRectangle (bounds, h * 0.5f);
    }

    juce::ColourGradient grad (juce::Colour (0xffefefef), knobRect.getTopLeft(),
                                juce::Colour (0xff8c8c8c), knobRect.getBottomRight(), false);
    g.setGradientFill (grad);
    g.fillEllipse (knobRect);
}

void ALXLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
    g.setColour (findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) width - 14.0f, ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 5.0f, ay - 3.0f, ax + 5.0f, ay - 3.0f, ax, ay + 4.0f);
    g.setColour (findColour (juce::ComboBox::textColourId));
    g.fillPath (arrow);
    juce::ignoreUnused (box);
}

//==============================================================================
ALX_LA2A_AudioProcessorEditor::ALX_LA2A_AudioProcessorEditor (ALX_LA2A_AudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (vuMeter);

    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible (s);

        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, juce::Colour (0xffd8d0b8));
        l.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        addAndMakeVisible (l);
    };

    setupKnob (peakReductionKnob, peakReductionLabel, "PEAK REDUCTION");
    setupKnob (gainKnob, gainLabel, "GAIN");

    peakReductionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "peakReduction", peakReductionKnob);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, "gain", gainKnob);

    addAndMakeVisible (powerSwitch);
    addAndMakeVisible (limitModeSwitch);

    powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorRef.apvts, "power", powerSwitch);
    limitModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorRef.apvts, "limitMode", limitModeSwitch);

    auto setupSmallLabel = [this] (juce::Label& l, const juce::String& text, float size = 11.0f)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, juce::Colour (0xffd8d0b8));
        l.setFont (juce::FontOptions (size, juce::Font::bold));
        addAndMakeVisible (l);
    };

    setupSmallLabel (powerLabelOff, "OFF");
    setupSmallLabel (powerLabelOn, "ON");
    setupSmallLabel (modeLabelOff, "COMPRESS");
    setupSmallLabel (modeLabelOn, "LIMIT");

    meterCombo.addItem ("GR", 1);
    meterCombo.addItem ("+4", 2);
    meterCombo.addItem ("+8", 3);
    addAndMakeVisible (meterCombo);
    meterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processorRef.apvts, "meter", meterCombo);

    setupSmallLabel (meterLabel, "METER");

    titleLabel.setText ("ALX-LA-2A", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffece4c8));
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("ELECTRO-OPTICAL ATTENUATOR", juce::dontSendNotification);
    subtitleLabel.setFont (juce::FontOptions (11.0f, juce::Font::plain));
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9a9078));
    subtitleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (subtitleLabel);

    setSize (660, 360);
    startTimerHz (30);
}

ALX_LA2A_AudioProcessorEditor::~ALX_LA2A_AudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ALX_LA2A_AudioProcessorEditor::drawScrew (juce::Graphics& g, float cx, float cy, float r)
{
    juce::ColourGradient grad (juce::Colour (0xffbababa), cx - r, cy - r, juce::Colour (0xff4a4a4a), cx + r, cy + r, false);
    g.setGradientFill (grad);
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (cx - r * 0.6f, cy, cx + r * 0.6f, cy, 1.4f);
}

void ALX_LA2A_AudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Тёмная брашированная металлическая панель
    juce::ColourGradient panelGrad (juce::Colour (0xff242220), bounds.getX(), bounds.getY(),
                                     juce::Colour (0xff141312), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (panelGrad);
    g.fillRect (bounds);

    // Тонкие горизонтальные риски (имитация брашированного металла)
    g.setColour (juce::Colours::white.withAlpha (0.02f));
    for (float yy = 0; yy < bounds.getHeight(); yy += 2.0f)
        g.drawLine (0, yy, bounds.getWidth(), yy, 1.0f);

    // Рамка панели
    g.setColour (juce::Colour (0xff050505));
    g.drawRect (bounds, 3.0f);

    const float r = 7.0f;
    drawScrew (g, 18, 18, r);
    drawScrew (g, bounds.getWidth() - 18, 18, r);
    drawScrew (g, 18, bounds.getHeight() - 18, r);
    drawScrew (g, bounds.getWidth() - 18, bounds.getHeight() - 18, r);

    // Разделительная линия под заголовком
    g.setColour (juce::Colour (0xff3a362c));
    g.drawLine (20.0f, 64.0f, bounds.getWidth() - 20.0f, 64.0f, 1.0f);
}

void ALX_LA2A_AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (36, 18);

    auto header = area.removeFromTop (40);
    titleLabel.setBounds (header.removeFromLeft (260));
    subtitleLabel.setBounds (header);

    area.removeFromTop (14);

    auto meterArea = area.removeFromLeft (300);
    vuMeter.setBounds (meterArea.removeFromTop (200));

    auto meterRow = meterArea.removeFromTop (meterArea.getHeight());
    meterRow = meterRow.removeFromTop (40);
    meterLabel.setBounds (meterRow.removeFromLeft (60));
    meterCombo.setBounds (meterRow.removeFromLeft (90).reduced (0, 6));

    area.removeFromLeft (24);

    auto knobsArea = area;
    auto knobRow = knobsArea.removeFromTop (190);
    auto prArea = knobRow.removeFromLeft (knobRow.getWidth() / 2);
    auto gnArea = knobRow;

    auto prLabelArea = prArea.removeFromBottom (20);
    auto gnLabelArea = gnArea.removeFromBottom (20);
    peakReductionLabel.setBounds (prLabelArea);
    gainLabel.setBounds (gnLabelArea);
    peakReductionKnob.setBounds (prArea.reduced (14));
    gainKnob.setBounds (gnArea.reduced (14));

    knobsArea.removeFromTop (14);
    auto switchRow = knobsArea.removeFromTop (60);

    auto powerArea = switchRow.removeFromLeft (switchRow.getWidth() / 2).reduced (12, 6);
    auto modeArea  = switchRow.reduced (12, 6);

    auto powerLabelsRow = powerArea.removeFromBottom (16);
    powerLabelOff.setBounds (powerLabelsRow.removeFromLeft (powerLabelsRow.getWidth() / 2));
    powerLabelOn.setBounds (powerLabelsRow);
    powerSwitch.setBounds (powerArea.reduced (powerArea.getWidth() * 0.18f, 4));

    auto modeLabelsRow = modeArea.removeFromBottom (16);
    modeLabelOff.setBounds (modeLabelsRow.removeFromLeft (modeLabelsRow.getWidth() / 2));
    modeLabelOn.setBounds (modeLabelsRow);
    limitModeSwitch.setBounds (modeArea.reduced (modeArea.getWidth() * 0.18f, 4));
}

void ALX_LA2A_AudioProcessorEditor::timerCallback()
{
    vuMeter.setGRdb (processorRef.currentGRdb.load());
    vuMeter.setOutLevelDb (processorRef.currentOutLevelDb.load());
    vuMeter.setMeterMode (meterCombo.getSelectedId() - 1);
}
