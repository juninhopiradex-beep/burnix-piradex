// =============================================================================
//  BURNIX by PIRADEX — tema "Orbita": grafite azul-noite, cyan eletrico, coral incandescente e violeta.
//  Widgets pequenos e reutilizaveis: botoes em pilula, linha de parametro (rotulo + barra + valor exato),
//  segmentos ligados a parametros de escolha. Todos com foco de teclado e duplo-clique para repor.
// =============================================================================
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

namespace bxui
{
const juce::Colour kBg      (0xff0a0b18), kDisc (0xff10122a), kLine (0xff1c2040), kLine2 (0xff2a2f4a), kHead (0xff161a33);
const juce::Colour kText    (0xffeceef6), kDim  (0xff9aa0b8), kFaint (0xff6e7493);
const juce::Colour kCoral   (0xffff7a5c), kCyan (0xff2ee6f0), kViolet (0xffa98bff), kAmber (0xffffb36b);

inline juce::Font font (float size, bool bold = false) { return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)); }
inline juce::Font mono (float size) { return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain)); }
inline juce::String T8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

inline juce::Colour bandColour (int b)
{
    static const juce::Colour c[] { kViolet, kCoral, kAmber, kCyan, juce::Colour (0xff6fd3ff), juce::Colour (0xffc7b6ff) };
    return c[juce::jlimit (0, 5, b)];
}

// ---- LookAndFeel: pilulas e barras finas -------------------------------------------------------------
class LnF : public juce::LookAndFeel_V4
{
public:
    LnF()
    {
        setColour (juce::PopupMenu::backgroundColourId, kDisc); setColour (juce::PopupMenu::textColourId, kText);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kLine2); setColour (juce::PopupMenu::highlightedTextColourId, kText);
        setColour (juce::TextEditor::backgroundColourId, kBg); setColour (juce::TextEditor::textColourId, kText);
        setColour (juce::TextEditor::outlineColourId, kLine2); setColour (juce::TextEditor::focusedOutlineColourId, kCyan);
        setColour (juce::ToggleButton::textColourId, kDim); setColour (juce::ToggleButton::tickColourId, kCyan);
        setColour (juce::TooltipWindow::backgroundColourId, kDisc); setColour (juce::TooltipWindow::textColourId, kText); setColour (juce::TooltipWindow::outlineColourId, kLine2);
    }
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        const auto acc = b.findColour (juce::TextButton::buttonOnColourId);
        if (on) { g.setColour (acc); g.fillRoundedRectangle (r, r.getHeight() / 2); }
        else
        {
            g.setColour (over ? kLine2.brighter (0.1f) : kBg.withAlpha (0.f)); g.fillRoundedRectangle (r, r.getHeight() / 2);
            g.setColour (b.hasKeyboardFocus (false) ? kCyan : kLine2); g.drawRoundedRectangle (r, r.getHeight() / 2, 1.f);
        }
        if (down) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRoundedRectangle (r, r.getHeight() / 2); }
    }
    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setColour (b.getToggleState() ? b.findColour (juce::TextButton::textColourOnId) : b.findColour (juce::TextButton::textColourOffId));
        g.setFont (font (juce::jmin (11.f, b.getHeight() * 0.42f), true));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 0), juce::Justification::centred, 1);
    }
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const float cy = y + h * 0.5f;
        g.setColour (kLine); g.fillRoundedRectangle ((float) x, cy - 2.f, (float) w, 4.f, 2.f);
        const auto c = s.findColour (juce::Slider::trackColourId);
        const bool bip = s.getProperties().getWithDefault ("bipolar", false);
        if (bip)
        {
            const float mid = x + w * 0.5f; g.setColour (c);
            g.fillRoundedRectangle (juce::jmin (mid, pos), cy - 2.f, std::abs (pos - mid), 4.f, 2.f);
        }
        else { g.setColour (c); g.fillRoundedRectangle ((float) x, cy - 2.f, juce::jmax (0.f, pos - x), 4.f, 2.f); }
        g.setColour (s.hasKeyboardFocus (false) ? kCyan : kText);
        g.fillEllipse (pos - 5.f, cy - 5.f, 10.f, 10.f);
    }
    juce::Font getPopupMenuFont() override { return font (13.f); }
};

inline void stylePill (juce::TextButton& b, juce::Colour on = kText, juce::Colour textOn = kBg)
{
    b.setColour (juce::TextButton::buttonOnColourId, on);
    b.setColour (juce::TextButton::textColourOnId, textOn);
    b.setColour (juce::TextButton::textColourOffId, kDim);
    b.setWantsKeyboardFocus (true);
}

// ---- linha de parametro: ROTULO ───────●──── VALOR ---------------------------------------------------
class ParamRow : public juce::Component
{
public:
    ParamRow (const juce::String& label, juce::Colour c, bool bipolar = false) : text (label)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal); slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::trackColourId, c); slider.getProperties().set ("bipolar", bipolar);
        slider.setWantsKeyboardFocus (true); slider.setVelocityModeParameters (0.8, 1, 0.05, true, juce::ModifierKeys::shiftModifier);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
    }
    void attach (juce::AudioProcessorValueTreeState& s, const juce::String& id)
    {
        att.reset(); att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
        if (auto* p = s.getParameter (id)) { slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue())); slider.setTooltip (p->getName (64) + T8 (" · duplo-clique repõe · Shift = ajuste fino")); }
        paramId = id; repaint();
    }
    void setLabel (const juce::String& l) { text = l; repaint(); }
    void setSuffix (const juce::String& s) { suffix = s; repaint(); }
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedLeft (labelW).withTrimmedRight (valueW)); }
    void paint (juce::Graphics& g) override
    {
        g.setColour (kDim); g.setFont (font (11.f, true));
        g.drawText (text, 0, 0, labelW - 6, getHeight(), juce::Justification::centredLeft);
        g.setColour (kText); g.setFont (mono (12.f));
        g.drawText (slider.getTextFromValue (slider.getValue()) + suffix, getWidth() - valueW, 0, valueW, getHeight(), juce::Justification::centredRight);
    }
    juce::Slider slider; int labelW = 96, valueW = 70;
private:
    juce::String text, suffix, paramId;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// ---- segmentos ligados a um parametro de escolha (ou a uma acao) --------------------------------------
class Segments : public juce::Component
{
public:
    Segments (juce::StringArray names, juce::Colour on = kText, juce::Colour textOn = kBg)
    {
        for (int i = 0; i < names.size(); ++i)
        {
            auto* b = btn.add (new juce::TextButton (names[i])); stylePill (*b, on, textOn); b->setClickingTogglesState (false);
            b->onClick = [this, i] { if (onPick) onPick (i); else setParamIndex (i); };
            addAndMakeVisible (b);
        }
    }
    void bind (juce::AudioProcessorValueTreeState& s, const juce::String& id) { param = s.getParameter (id); refresh(); }
    void unbind() { param = nullptr; }
    void setParamIndex (int i)
    {
        if (param == nullptr) return;
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param); if (rp == nullptr) return;
        rp->beginChangeGesture(); rp->setValueNotifyingHost (rp->convertTo0to1 ((float) i)); rp->endChangeGesture(); refresh();
    }
    void refresh()
    {
        int idx = selected;
        if (param != nullptr) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param)) idx = (int) std::lround (rp->convertFrom0to1 (rp->getValue()));
        for (int i = 0; i < btn.size(); ++i) btn[i]->setToggleState (i == idx, juce::dontSendNotification);
    }
    void setSelected (int i) { selected = i; refresh(); }
    void resized() override
    {
        auto r = getLocalBounds(); const int n = btn.size(); if (n == 0) return;
        const int gap = 5, w = (r.getWidth() - gap * (n - 1)) / n;
        for (int i = 0; i < n; ++i) btn[i]->setBounds (r.getX() + i * (w + gap), r.getY(), i == n - 1 ? r.getRight() - (r.getX() + i * (w + gap)) : w, r.getHeight());
    }
    std::function<void (int)> onPick;
    juce::OwnedArray<juce::TextButton> btn;
private:
    juce::AudioProcessorParameter* param = nullptr; int selected = -1;
};

// ---- botao ligado a um parametro booleano ---------------------------------------------------------------
class ToggleParam : public juce::TextButton
{
public:
    ToggleParam (const juce::String& t, juce::Colour on) : juce::TextButton (t) { stylePill (*this, on, kBg); setClickingTogglesState (true); }
    void attach (juce::AudioProcessorValueTreeState& s, const juce::String& id) { att.reset(); att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this); }
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};
} // namespace bxui
