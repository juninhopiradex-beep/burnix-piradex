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
// ---- paleta ativa (mutavel: a roda dentada troca o esquema de cores em todas as janelas abertas) ------------
inline juce::Colour kBg { 0xff0a0b18 }, kDisc { 0xff10122a }, kLine { 0xff1c2040 }, kLine2 { 0xff2a2f4a }, kHead { 0xff161a33 }, kText { 0xffeceef6 },
                    kDim { 0xff9aa0b8 }, kFaint { 0xff797f9b }, kCoral { 0xffff7a5c }, kCyan { 0xff2ee6f0 }, kViolet { 0xffa98bff }, kAmber { 0xffffb36b };
inline int gSkin = 0;
struct Skin { const char* pt; const char* en; juce::uint32 c[12]; };
inline const Skin* skins()
{
    //                                   bg          disc        line        line2       head        text        dim         faint       coral(1.o)  cyan(2.o)   violet      amber
    static const Skin s[5] {
        { "Órbita",            "Orbit",            { 0xff0a0b18, 0xff10122a, 0xff1c2040, 0xff2a2f4a, 0xff161a33, 0xffeceef6, 0xff9aa0b8, 0xff797f9b, 0xffff7a5c, 0xff2ee6f0, 0xffa98bff, 0xffffb36b } },
        { "Brasa",             "Ember",            { 0xff120a09, 0xff1d1210, 0xff2e1c18, 0xff402823, 0xff251614, 0xfff7ece6, 0xffb79f96, 0xff927d76, 0xffff5a36, 0xffffb347, 0xffe8609c, 0xffffd27a } },
        { "Gelo",              "Ice",              { 0xff07111a, 0xff0c1a27, 0xff17293a, 0xff23394f, 0xff10202e, 0xffe8f4fb, 0xff94b0c3, 0xff6f8697, 0xff63c7ff, 0xffa6f0ff, 0xff8f9cff, 0xffd8f0ff } },
        { "Índigo (daltonismo)", "Indigo (colour-blind safe)", { 0xff0b0d1c, 0xff12152e, 0xff1e2346, 0xff2c3256, 0xff171b38, 0xffeef0f8, 0xffa0a6c0, 0xff7a80a0, 0xffff9f1c, 0xff3da5ff, 0xff9db4ff, 0xffffd166 } },
        { "Clara",             "Light",            { 0xfff3f1ec, 0xffffffff, 0xffe0ddd5, 0xffc9c5bb, 0xffe7e4dd, 0xff15161c, 0xff575a6a, 0xff696b79, 0xffd2442a, 0xff0a8597, 0xff6249d6, 0xffb06d0c } },
    };
    return s;
}
inline void applySkin (int i)
{
    gSkin = juce::jlimit (0, 4, i); const auto& c = skins()[gSkin].c;
    juce::Colour* t[12] { &kBg, &kDisc, &kLine, &kLine2, &kHead, &kText, &kDim, &kFaint, &kCoral, &kCyan, &kViolet, &kAmber };
    for (int k = 0; k < 12; ++k) *t[k] = juce::Colour (c[k]);
}

// ---- idioma: 0 = PT, 1 = EN (os rotulos tecnicos — HEAT, DRIVE, MIX... — sao iguais nas duas)
inline int gLang = 0;
inline juce::String L (const char* pt, const char* en) { return gLang == 1 ? juce::String (juce::CharPointer_UTF8 (en)) : juce::String (juce::CharPointer_UTF8 (pt)); }

inline juce::Font font (float size, bool bold = false) { return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)); }
inline juce::Font mono (float size) { return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain)); }
inline juce::String T8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

inline juce::Colour bandColour (int b)
{
    switch (juce::jlimit (0, 5, b))
    {
        case 0: return kViolet; case 1: return kCoral; case 2: return kAmber; case 3: return kCyan;
        case 4: return kCyan.interpolatedWith (kText, 0.35f); default: return kViolet.interpolatedWith (kText, 0.35f);
    }
}

// papeis de cor: os widgets guardam o PAPEL (nao a cor), para poderem ser repintados quando o esquema muda
inline juce::Colour roleColour (int r)
{
    switch (r) { case 0: return kText; case 1: return kCoral; case 2: return kCyan; case 3: return kViolet; case 4: return kAmber; case 5: return kBg; default: return bandColour (r - 10); }
}
inline int roleOf (juce::Colour c)
{
    for (int r : { 0, 1, 2, 3, 4, 5 }) if (roleColour (r) == c) return r;
    for (int b = 0; b < 6; ++b) if (bandColour (b) == c) return 10 + b;
    return 0;
}

// ---- logotipo: "BURN" em vermelho fogo (gradiente vermelho -> laranja), "IX" na cor do texto, "by PIRADEX" logo a seguir.
//      As larguras sao medidas com a fonte real (no Mac a fonte do sistema e diferente da do Linux/Windows).
inline float logoWidth (float size, bool withBy)
{
    const auto f = font (size, true); const float track = size * 0.24f;
    float w = 0; for (auto ch : juce::String ("BURNIX")) w += juce::GlyphArrangement::getStringWidth (f, juce::String::charToString (ch)) + track;
    w -= track;
    if (withBy) w += size * 0.45f + juce::GlyphArrangement::getStringWidth (font (size * 0.48f, true), "by PIRADEX");
    return w;
}
// desenha com a linha de base em baselineY; x e o inicio (ou o centro, se centred)
inline void drawLogo (juce::Graphics& g, float x, float baselineY, float size, bool centred, bool withBy = true)
{
    const auto f = font (size, true); const float track = size * 0.24f;
    if (centred) x -= logoWidth (size, withBy) * 0.5f;
    const juce::String word ("BURNIX");
    const float x0 = x, burnW = [&] { float w = 0; for (int i = 0; i < 4; ++i) w += juce::GlyphArrangement::getStringWidth (f, word.substring (i, i + 1)) + track; return w - track; }();
    for (int i = 0; i < word.length(); ++i)
    {
        const auto ch = word.substring (i, i + 1);
        if (i < 4) g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff1f0f), x0, baselineY - size, juce::Colour (0xffff7a00), x0 + burnW, baselineY, false));
        else g.setColour (kText);
        juce::GlyphArrangement ga; ga.addLineOfText (f, ch, x, baselineY); ga.draw (g);
        x += juce::GlyphArrangement::getStringWidth (f, ch) + track;
    }
    if (withBy)
    {
        x += size * 0.45f - track;
        g.setColour (kDim); juce::GlyphArrangement ga; ga.addLineOfText (font (size * 0.48f, true), "by PIRADEX", x, baselineY); ga.draw (g);
    }
}

// ---- LookAndFeel: pilulas e barras finas -------------------------------------------------------------
class LnF : public juce::LookAndFeel_V4
{
public:
    LnF() { refresh(); }
    void refresh()
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
        if (down) { g.setColour (kText.withAlpha (0.08f)); g.fillRoundedRectangle (r, r.getHeight() / 2); }
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

inline void stylePillRoles (juce::TextButton& b, int onRole, int textRole)
{
    b.getProperties().set ("onRole", onRole); b.getProperties().set ("textRole", textRole);
    b.setColour (juce::TextButton::buttonOnColourId, roleColour (onRole));
    b.setColour (juce::TextButton::textColourOnId, roleColour (textRole));
    b.setColour (juce::TextButton::textColourOffId, kDim);
    b.setWantsKeyboardFocus (true);
}
inline void stylePill (juce::TextButton& b, juce::Colour on = kText, juce::Colour textOn = kBg) { stylePillRoles (b, roleOf (on), roleOf (textOn)); }

// reaplica as cores (por papel) a uma arvore de componentes depois de mudar o esquema
inline void restyleTree (juce::Component& root)
{
    if (auto* b = dynamic_cast<juce::TextButton*> (&root))
        if (b->getProperties().contains ("onRole")) stylePillRoles (*b, b->getProperties()["onRole"], b->getProperties()["textRole"]);
    if (auto* sl = dynamic_cast<juce::Slider*> (&root))
        if (sl->getProperties().contains ("trackRole")) sl->setColour (juce::Slider::trackColourId, roleColour (sl->getProperties()["trackRole"]));
    for (auto* c : root.getChildren()) restyleTree (*c);
    root.repaint();
}

// ---- linha de parametro: ROTULO ───────●──── VALOR ---------------------------------------------------
class ParamRow : public juce::Component
{
public:
    ParamRow (const juce::String& label, juce::Colour c, bool bipolar = false) : text (label)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal); slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::trackColourId, c); slider.getProperties().set ("bipolar", bipolar); slider.getProperties().set ("trackRole", roleOf (c));
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
            b->onClick = [this, i] { if (onPick) onPick (i); else setParamIndex (valueOf (i)); };
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
        for (int i = 0; i < btn.size(); ++i) btn[i]->setToggleState ((param != nullptr ? valueOf (i) : i) == idx, juce::dontSendNotification);
    }
    void setSelected (int i) { selected = i; refresh(); }
    void setValueMap (std::vector<int> m) { map = std::move (m); refresh(); }   // botao i -> indice do parametro
    int valueOf (int i) const { return i < (int) map.size() ? map[(size_t) i] : i; }
    void resized() override
    {
        auto r = getLocalBounds(); const int n = btn.size(); if (n == 0) return;
        const int gap = 5, w = (r.getWidth() - gap * (n - 1)) / n;
        for (int i = 0; i < n; ++i) btn[i]->setBounds (r.getX() + i * (w + gap), r.getY(), i == n - 1 ? r.getRight() - (r.getX() + i * (w + gap)) : w, r.getHeight());
    }
    std::function<void (int)> onPick;
    juce::OwnedArray<juce::TextButton> btn;
private:
    juce::AudioProcessorParameter* param = nullptr; int selected = -1; std::vector<int> map;
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
// ---- botao rotativo grande (HEAT, MIX): arco de 270 graus, valor grande no centro, rotulo por baixo -------------
class BigKnob : public juce::Slider
{
public:
    BigKnob (const juce::String& lbl, juce::Colour c) : label (lbl)
    {
        setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (juce::degreesToRadians (225.f), juce::degreesToRadians (495.f), true);
        getProperties().set ("trackRole", roleOf (c)); setColour (juce::Slider::trackColourId, c);
        setVelocityModeParameters (0.8, 1, 0.05, true, juce::ModifierKeys::shiftModifier); setWantsKeyboardFocus (true);
    }
    void attach (juce::AudioProcessorValueTreeState& s, const juce::String& id)
    {
        att.reset(); att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, *this);
        if (auto* p = s.getParameter (id)) { setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue())); setTooltip (p->getName (64) + T8 (" · duplo-clique repõe · Shift = ajuste fino")); }
    }
    std::function<juce::String (double)> format;
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat(); const float sz = juce::jmin (r.getWidth(), r.getHeight() - 18.f);
        const juce::Point<float> c (r.getCentreX(), sz * 0.5f + 2.f); const float rad = sz * 0.5f - 8.f;
        const auto pr = (float) valueToProportionOfLength (getValue());
        const float a0 = juce::degreesToRadians (-135.f), a1 = juce::degreesToRadians (135.f);
        juce::Path tr; tr.addCentredArc (c.x, c.y, rad, rad, 0, a0, a1, true);
        g.setColour (kLine); g.strokePath (tr, juce::PathStrokeType (9.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path fl; fl.addCentredArc (c.x, c.y, rad, rad, 0, a0, a0 + (a1 - a0) * pr, true);
        g.setColour (findColour (juce::Slider::trackColourId)); g.strokePath (fl, juce::PathStrokeType (9.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (kText); g.setFont (font (sz * 0.25f));
        g.drawText (format ? format (getValue()) : getTextFromValue (getValue()), juce::Rectangle<float> (sz, sz * 0.3f).withCentre (c).toNearestInt(), juce::Justification::centred);
        g.setColour (hasKeyboardFocus (false) ? kCyan : kDim); g.setFont (font (10.f, true));
        g.drawText (label, juce::Rectangle<float> (r.getX(), r.getBottom() - 16.f, r.getWidth(), 14.f).toNearestInt(), juce::Justification::centred);
    }
private:
    juce::String label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// ---- moldura de grupo (cartao com titulo), desenhada pelo painel que a contem ---------------------------------
inline void drawCard (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour (kLine); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 11.f, 1.f);
    g.setColour (kDim); g.setFont (font (10.f, true)); g.drawText (title, r.getX() + 12, r.getY() + 7, r.getWidth() - 24, 14, juce::Justification::left);
}
} // namespace bxui
