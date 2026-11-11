#include "PluginEditor.h"

namespace
{
    const juce::Colour pink   { 0xffff6fb0 };
    const juce::Colour pinkLt { 0xffffc2de };
    const juce::Colour lilac  { 0xffb79bff };
    const juce::Colour plum   { 0xff7a3f99 };
    const juce::Colour ink    { 0xff5a3a6b };

    constexpr float halfPi = juce::MathConstants<float>::halfPi;

    juce::Path heartPath (float cx, float cy, float size)
    {
        juce::Path p;
        p.startNewSubPath (0.0f, 0.9f);
        p.cubicTo (-1.3f, 0.0f, -1.0f, -1.0f, 0.0f, -0.35f);
        p.cubicTo (1.0f, -1.0f, 1.3f, 0.0f, 0.0f, 0.9f);
        p.closeSubPath();
        p.applyTransform (juce::AffineTransform::scale (size).translated (cx, cy));
        return p;
    }

    juce::Path sparklePath (float cx, float cy, float size)
    {
        juce::Path p;
        p.addStar ({ cx, cy }, 4, size * 0.22f, size, 0.0f);
        return p;
    }

    juce::Font boldFont (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
}

//==============================================================================
EvvscyLookAndFeel::EvvscyLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, plum);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, plum);
    setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
}

void EvvscyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float pos, float startA, float endA, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float radius = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startA + pos * (endA - startA);
    const float arcR = radius - 3.0f;

    // дорожка
    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startA, endA, true);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.strokePath (track, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // значение
    juce::Path val;
    val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startA, angle, true);
    g.setGradientFill (juce::ColourGradient (pink, c.x - arcR, c.y, lilac, c.x + arcR, c.y, false));
    g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // тело ручки
    const float bodyR = radius - 11.0f;
    g.setColour (pink.withAlpha (0.25f));
    g.fillEllipse (c.x - bodyR, c.y - bodyR + 3.0f, bodyR * 2.0f, bodyR * 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, c.x, c.y - bodyR,
                                             juce::Colour (0xffffe1f0), c.x, c.y + bodyR, false));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (pinkLt);
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 2.0f);

    // мордочка
    for (float side : { -1.0f, 1.0f })
    {
        const float ex = c.x + side * bodyR * 0.42f;
        const float ey = c.y - bodyR * 0.1f;

        g.setColour (ink);
        g.fillEllipse (ex - bodyR * 0.09f, ey - bodyR * 0.12f, bodyR * 0.18f, bodyR * 0.24f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (ex - bodyR * 0.04f, ey - bodyR * 0.09f, bodyR * 0.07f, bodyR * 0.07f);

        g.setColour (pink.withAlpha (0.35f));
        g.fillEllipse (c.x + side * bodyR * 0.62f - bodyR * 0.15f, c.y + bodyR * 0.22f - bodyR * 0.085f,
                       bodyR * 0.3f, bodyR * 0.17f);
    }

    juce::Path mouth;
    mouth.addCentredArc (c.x, c.y + bodyR * 0.18f, bodyR * 0.1f, bodyR * 0.1f, 0.0f, halfPi, 3.0f * halfPi, true);
    g.setColour (ink);
    g.strokePath (mouth, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // указатель-сердечко
    const float px = c.x + arcR * std::sin (angle);
    const float py = c.y - arcR * std::cos (angle);
    g.setColour (juce::Colours::white);
    g.fillEllipse (px - 6.0f, py - 6.0f, 12.0f, 12.0f);
    g.setColour (pink);
    g.fillPath (heartPath (px, py, 4.2f));
}

void EvvscyLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool highlighted, bool)
{
    auto r = button.getLocalBounds().toFloat().reduced (2.0f);
    const bool on = button.getToggleState();
    const float corner = r.getHeight() * 0.5f;

    g.setColour (on ? pink : juce::Colours::white.withAlpha (0.85f));
    g.fillRoundedRectangle (r, corner);
    g.setColour (pink.withAlpha (highlighted ? 1.0f : 0.8f));
    g.drawRoundedRectangle (r, corner, 2.0f);

    g.setColour (on ? pinkLt : pink);
    g.fillPath (heartPath (r.getX() + corner, r.getCentreY(), 5.0f));

    g.setColour (on ? juce::Colours::white : plum);
    g.setFont (boldFont (15.0f));
    g.drawText (on ? "Delta: ON" : "Delta: off", r.withTrimmedLeft (corner + 8.0f),
                juce::Justification::centred);
}

//==============================================================================
EvvscyEditor::EvvscyEditor (EvvscySupressorProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    static const char* ids[9]    = { "depth", "threshold", "width", "attack", "release", "maxred", "lowfreq", "highfreq", "mix" };
    static const char* names[9]  = { "Depth", "Threshold", "Width", "Attack", "Release", "Max Cut", "Low Freq", "High Freq", "Mix" };
    static const char* suffix[9] = { " %", " dB", " oct", " ms", " ms", " dB", " Hz", " Hz", " %" };

    for (int i = 0; i < 9; ++i)
    {
        auto& k = knobs[(size_t) i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 18);
        k.slider.setTextValueSuffix (suffix[i]);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (boldFont (15.0f));
        addAndMakeVisible (k.label);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.getApvts(), ids[i], k.slider);
    }

    addAndMakeVisible (deltaButton);
    deltaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.getApvts(), "delta", deltaButton);

    setSize (660, 510);
    startTimerHz (30);
}

EvvscyEditor::~EvvscyEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void EvvscyEditor::timerCallback()
{
    phase += 0.033f;
    ++frame;
    shownReduction += (proc.getReductionDb() - shownReduction) * 0.2f;
    repaint();
}

void EvvscyEditor::resized()
{
    constexpr int originX = 20, originY = 166, cellW = 124, cellH = 150;

    for (int i = 0; i < 10; ++i)
    {
        juce::Rectangle<int> cell (originX + (i % 5) * cellW, originY + (i / 5) * cellH, cellW, cellH);

        if (i < 9)
        {
            auto& k = knobs[(size_t) i];
            k.label.setBounds (cell.removeFromTop (22));
            k.slider.setBounds (cell);
        }
        else
        {
            deltaButton.setBounds (cell.withSizeKeepingCentre (112, 38));
        }
    }
}

//==============================================================================
void EvvscyEditor::drawMascot (juce::Graphics& g, juce::Rectangle<float> area)
{
    auto c = area.getCentre();
    const float r = area.getWidth() * 0.36f;
    c.y += std::sin (phase * 3.0f) * 2.0f;   // лёгкое покачивание

    const juce::Colour hair (0xffff9ccd), hairDark (0xffff7ab8), skin (0xfffff1ea);
    const bool happy = shownReduction > 0.8f;
    const bool blink = (frame % 160) < 5;

    // хвостики
    g.setColour (hair);
    g.fillEllipse (c.x - r * 1.5f, c.y - r * 0.1f, r * 0.75f, r * 1.55f);
    g.fillEllipse (c.x + r * 0.75f, c.y - r * 0.1f, r * 0.75f, r * 1.55f);

    // кошачьи ушки
    juce::Path ear, earIn;
    ear.addTriangle (c.x - r * 0.95f, c.y - r * 0.3f, c.x - r * 0.85f, c.y - r * 1.4f, c.x - r * 0.1f, c.y - r * 0.85f);
    earIn.addTriangle (c.x - r * 0.8f, c.y - r * 0.5f, c.x - r * 0.78f, c.y - r * 1.12f, c.x - r * 0.3f, c.y - r * 0.82f);
    const auto mirror = juce::AffineTransform::scale (-1.0f, 1.0f, c.x, c.y);

    g.setColour (hair);
    g.fillPath (ear);
    g.setColour (pinkLt);
    g.fillPath (earIn);
    g.setColour (hair);
    g.fillPath (ear, mirror);
    g.setColour (pinkLt);
    g.fillPath (earIn, mirror);

    // голова
    juce::Rectangle<float> head (c.x - r, c.y - r * 0.95f, r * 2.0f, r * 1.9f);
    g.setColour (skin);
    g.fillEllipse (head);

    // чёлка
    g.saveState();
    juce::Path headClip;
    headClip.addEllipse (head);
    g.reduceClipRegion (headClip);
    g.setColour (hair);
    g.fillRect (head.getX(), head.getY(), head.getWidth(), r * 0.55f);
    for (int i = 0; i < 4; ++i)
        g.fillEllipse (head.getX() + (float) i * r * 0.5f - r * 0.05f, c.y - r * 0.8f, r * 0.6f, r * 0.75f);
    g.setColour (hairDark.withAlpha (0.5f));
    g.fillRect (head.getX(), head.getY(), head.getWidth(), r * 0.12f);
    g.restoreState();

    g.setColour (pink);
    g.drawEllipse (head, 2.5f);

    // глаза
    const float ey = c.y + r * 0.12f, ex = r * 0.42f;
    for (float side : { -1.0f, 1.0f })
    {
        const float x = c.x + side * ex;

        if (happy)
        {
            juce::Path arc;
            arc.addCentredArc (x, ey + r * 0.08f, r * 0.17f, r * 0.17f, 0.0f, -halfPi, halfPi, true);
            g.setColour (ink);
            g.strokePath (arc, juce::PathStrokeType (r * 0.075f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (blink)
        {
            g.setColour (ink);
            g.drawLine (x - r * 0.16f, ey, x + r * 0.16f, ey, r * 0.07f);
        }
        else
        {
            g.setColour (ink);
            g.fillEllipse (x - r * 0.15f, ey - r * 0.22f, r * 0.3f, r * 0.44f);
            g.setColour (juce::Colour (0xffb06bff));
            g.fillEllipse (x - r * 0.11f, ey - r * 0.02f, r * 0.22f, r * 0.19f);
            g.setColour (juce::Colours::white);
            g.fillEllipse (x - r * 0.1f, ey - r * 0.17f, r * 0.12f, r * 0.12f);
            g.fillEllipse (x + r * 0.03f, ey + r * 0.07f, r * 0.06f, r * 0.06f);
        }
    }

    // румянец
    g.setColour (pink.withAlpha (juce::jmin (0.85f, 0.35f + shownReduction * 0.05f)));
    g.fillEllipse (c.x - r * 0.62f - r * 0.17f, c.y + r * 0.4f - r * 0.1f, r * 0.34f, r * 0.2f);
    g.fillEllipse (c.x + r * 0.62f - r * 0.17f, c.y + r * 0.4f - r * 0.1f, r * 0.34f, r * 0.2f);

    // ротик "ω"
    juce::Path mouth;
    const float mr = r * 0.07f;
    mouth.addCentredArc (c.x - mr, c.y + r * 0.42f, mr, mr, 0.0f, halfPi, 3.0f * halfPi, true);
    mouth.addCentredArc (c.x + mr, c.y + r * 0.42f, mr, mr, 0.0f, halfPi, 3.0f * halfPi, true);
    g.setColour (ink);
    g.strokePath (mouth, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // бантик
    const float bx = c.x + r * 0.72f, by = c.y - r * 0.78f, s = r * 0.42f;
    juce::Path bow;
    bow.addTriangle (bx, by, bx + s, by - s * 0.6f, bx + s, by + s * 0.6f);
    bow.addTriangle (bx, by, bx - s, by - s * 0.6f, bx - s, by + s * 0.6f);
    g.setColour (juce::Colour (0xffff4f93));
    g.fillPath (bow);
    g.setColour (juce::Colour (0xffff2f7f));
    g.fillEllipse (bx - s * 0.28f, by - s * 0.28f, s * 0.56f, s * 0.56f);
}

void EvvscyEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // фон
    juce::ColourGradient bg (juce::Colour (0xffffd9ec), 0.0f, 0.0f, juce::Colour (0xffcfe3ff), 0.0f, b.getBottom(), false);
    bg.addColour (0.5, juce::Colour (0xffe6d6ff));
    g.setGradientFill (bg);
    g.fillAll();

    // горошек
    g.setColour (juce::Colours::white.withAlpha (0.28f));
    for (int row = 0; row * 44 < getHeight() + 44; ++row)
        for (int x = (row % 2) * 22; x < getWidth() + 44; x += 44)
            g.fillEllipse ((float) x, (float) (row * 44), 10.0f, 10.0f);

    // мерцающие звёздочки
    static const float sparkles[][3] = { { 40, 172, 9 }, { 612, 186, 11 }, { 330, 160, 7 }, { 36, 332, 8 }, { 622, 352, 10 },
                                         { 300, 484, 8 }, { 566, 168, 6 }, { 150, 162, 6 }, { 80, 486, 9 }, { 604, 492, 7 } };
    for (int i = 0; i < 10; ++i)
    {
        const float tw = 0.5f + 0.5f * std::sin (phase * 2.0f + (float) i * 1.7f);
        g.setColour (juce::Colours::white.withAlpha (0.35f + 0.6f * tw));
        g.fillPath (sparklePath (sparkles[i][0], sparkles[i][1], sparkles[i][2] * (0.8f + 0.3f * tw)));
    }

    // сердечки
    g.setColour (pinkLt.withAlpha (0.9f));
    g.fillPath (heartPath (604.0f, 172.0f, 7.0f));
    g.fillPath (heartPath (24.0f, 470.0f, 6.0f));
    g.fillPath (heartPath (640.0f, 300.0f, 6.0f));

    // шапка
    auto card = juce::Rectangle<float> (12.0f, 12.0f, 636.0f, 142.0f);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.fillRoundedRectangle (card, 28.0f);
    g.setColour (pinkLt);
    g.drawRoundedRectangle (card, 28.0f, 3.0f);

    drawMascot (g, { 24.0f, 18.0f, 130.0f, 130.0f });

    // название
    g.setFont (boldFont (34.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("EvvscySupressor", 167, 26, 380, 44, juce::Justification::centredLeft);
    g.setColour (pink);
    g.drawText ("EvvscySupressor", 165, 24, 380, 44, juce::Justification::centredLeft);

    g.setFont (juce::Font (juce::FontOptions (15.0f)));
    g.setColour (lilac);
    g.drawText ("~ kawaii resonance suppressor ~", 165, 68, 340, 20, juce::Justification::centredLeft);

    // индикатор "прижато резонансов"
    g.setFont (boldFont (13.0f));
    g.setColour (plum);
    g.drawText ("squished: " + juce::String (shownReduction, 1) + " dB", 165, 92, 340, 16, juce::Justification::centredLeft);

    juce::Rectangle<float> bar (165.0f, 112.0f, 340.0f, 16.0f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.fillRoundedRectangle (bar, 8.0f);
    const float ratio = juce::jlimit (0.0f, 1.0f, shownReduction / 12.0f);
    if (ratio > 0.01f)
    {
        auto fill = bar.withWidth (juce::jmax (16.0f, bar.getWidth() * ratio));
        g.setGradientFill (juce::ColourGradient (pink, fill.getX(), 0.0f, lilac, fill.getRight(), 0.0f, false));
        g.fillRoundedRectangle (fill, 8.0f);
    }
    g.setColour (pinkLt);
    g.drawRoundedRectangle (bar, 8.0f, 2.0f);

    // бейдж evvscy
    juce::Rectangle<float> badge (532.0f, 24.0f, 104.0f, 30.0f);
    g.setGradientFill (juce::ColourGradient (pink, badge.getX(), 0.0f, lilac, badge.getRight(), 0.0f, false));
    g.fillRoundedRectangle (badge, 15.0f);
    g.setColour (juce::Colours::white);
    g.fillPath (sparklePath (badge.getX() + 17.0f, badge.getCentreY(), 7.0f));
    g.setFont (boldFont (17.0f));
    g.drawText ("evvscy", badge.withTrimmedLeft (22.0f), juce::Justification::centred);

    // подвал
    g.setFont (boldFont (14.0f));
    g.setColour (plum);
    g.drawText ("made with love by evvscy", 0, 478, getWidth(), 24, juce::Justification::centred);
    g.setColour (pink);
    g.fillPath (heartPath (getWidth() * 0.5f - 105.0f, 490.0f, 5.0f));
    g.fillPath (heartPath (getWidth() * 0.5f + 105.0f, 490.0f, 5.0f));
}
