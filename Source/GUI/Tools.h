// =============================================================================
//  BURNIX by PIRADEX — 2.0: painel MOTION (modulacao, sidechain, A/B morph) e ferramentas em janela:
//  BURNIX Link (todas as instancias da sessao), Assistente (ouve 10 s e sugere presets) e Match de referencia
//  (densidade por banda de uma faixa de referencia -> drive por banda).
// =============================================================================
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include "Theme.h"
#include "Panels.h"

namespace bxui
{
// ------------------------------------------------------------------------------------------- MOTION
class MotionPanel : public juce::Component
{
public:
    explicit MotionPanel (BurnixProcessor& pr) : p (pr)
    {
        for (int m = 0; m < 3; ++m)
        {
            src[m].bind (p.apvts, "mod" + juce::String (m + 1) + "Src"); dst[m].bind (p.apvts, "mod" + juce::String (m + 1) + "Dst");
            amt[m].attach (p.apvts, "mod" + juce::String (m + 1) + "Amt"); amt[m].setSuffix (" %"); amt[m].labelW = 104;
            for (auto* c : std::initializer_list<juce::Component*> { &src[m], &dst[m], &amt[m] }) addAndMakeVisible (c);
        }
        rate.attach (p.apvts, "lfoRate"); rate.setSuffix (" Hz"); shape.bind (p.apvts, "lfoShape");
        atk.attach (p.apvts, "envAttack"); atk.setSuffix (" ms"); rel.attach (p.apvts, "envRelease"); rel.setSuffix (" ms");
        morph.attach (p.apvts, "abMorph"); morph.setSuffix (" %");
        for (auto* r : { &rate, &atk, &rel, &morph }) r->labelW = 84;
        for (auto* c : std::initializer_list<juce::Component*> { &rate, &shape, &atk, &rel, &morph }) addAndMakeVisible (c);
        relabel();
    }
    void relabel()
    {
        for (int m = 0; m < 3; ++m) { src[m].btn[0]->setButtonText (L ("DESL.", "OFF")); amt[m].setLabel (L ("QUANTIDADE", "AMOUNT")); }
        morph.setLabel (T8 ("A → B"));
        morph.slider.setTooltip (L ("Leva o som atual até ao outro lado do A/B (automatizável: verso → refrão)", "Moves the current sound towards the other A/B side (automatable: verse → chorus)"));
        repaint();
    }
    void refresh() { for (int m = 0; m < 3; ++m) { src[m].refresh(); dst[m].refresh(); } shape.refresh(); repaint(); }
    void resized() override
    {
        auto r = getLocalBounds();
        cardL = r.removeFromTop (28 + 3 * 92 + 2); auto a = cardL.reduced (10, 0).withTrimmedTop (28);
        for (int m = 0; m < 3; ++m)
        {
            auto s = a.removeFromTop (88); s.removeFromLeft (18);
            src[m].setBounds (s.removeFromTop (26)); s.removeFromTop (4); dst[m].setBounds (s.removeFromTop (26)); s.removeFromTop (4); amt[m].setBounds (s.removeFromTop (24));
            a.removeFromTop (4);
        }
        r.removeFromTop (8);
        cardF = r.removeFromTop (28 + 24 + 6 + 26 + 8); auto b = cardF.reduced (10, 0).withTrimmedTop (28);
        rate.setBounds (b.removeFromTop (24)); b.removeFromTop (6); shape.setBounds (b.removeFromTop (26));
        r.removeFromTop (8);
        cardE = r.removeFromTop (28 + 24 + 4 + 24 + 22); auto c = cardE.reduced (10, 0).withTrimmedTop (28);
        atk.setBounds (c.removeFromTop (24)); c.removeFromTop (4); rel.setBounds (c.removeFromTop (24));
        r.removeFromTop (8);
        cardM = r.removeFromTop (28 + 24 + 8); morph.setBounds (cardM.reduced (10, 0).withTrimmedTop (28).removeFromTop (24));
    }
    void paint (juce::Graphics& g) override
    {
        drawCard (g, cardL, L ("LIGAÇÕES DE MODULAÇÃO", "MODULATION ROUTINGS")); drawCard (g, cardF, "LFO");
        drawCard (g, cardE, L ("ENVOLVENTE (ENV e SIDECHAIN)", "ENVELOPE (ENV and SIDECHAIN)")); drawCard (g, cardM, L ("A/B MORPH", "A/B MORPH"));
        // fontes ao vivo, no cabecalho do primeiro cartao
        const float v[3] { p.modLive.env.load(), 0.5f + 0.5f * p.modLive.lfo.load(), p.modLive.sc.load() };
        const char* n[3] { "ENV", "LFO", "SC" }; const juce::Colour c[3] { kCyan, kViolet, kAmber };
        for (int i = 0; i < 3; ++i)
        {
            const int x = cardL.getRight() - 186 + i * 60, y = cardL.getY() + 9;
            g.setColour (kDim); g.setFont (font (9.f, true)); g.drawText (n[i], x, y - 1, 22, 12, juce::Justification::left);
            g.setColour (kLine); g.fillRoundedRectangle ((float) x + 22, (float) y + 2, 32, 6, 3); g.setColour (c[i]); g.fillRoundedRectangle ((float) x + 22, (float) y + 2, 32 * juce::jlimit (0.f, 1.f, v[i]), 6, 3);
        }
        for (int m = 0; m < 3; ++m) { g.setColour (kDim); g.setFont (font (13.f, true)); g.drawText (juce::String (m + 1), cardL.getX() + 10, src[m].getY() + 4, 14, 18, juce::Justification::left); }
        g.setColour (p.modLive.scConnected.load() ? kCyan : kDim); g.setFont (font (10.5f));
        g.drawText (p.modLive.scConnected.load() ? L ("Sidechain ligado: o SC segue a pista de controlo.", "Sidechain connected: SC follows the key track.")
                                                 : L ("Sidechain desligado: liga uma pista ao sidechain na DAW.", "No sidechain: route a key track to the sidechain in your DAW."),
                    cardE.getX() + 12, cardE.getBottom() - 20, cardE.getWidth() - 24, 14, juce::Justification::left);
    }
private:
    BurnixProcessor& p; juce::Rectangle<int> cardL, cardF, cardE, cardM;
    Segments src[3] { Segments ({ "DESL.", "ENV", "LFO", "SC" }), Segments ({ "DESL.", "ENV", "LFO", "SC" }), Segments ({ "DESL.", "ENV", "LFO", "SC" }) };
    Segments dst[3] { Segments ({ "HEAT", "MORPH X", "MORPH Y", "MIX", "DENS" }), Segments ({ "HEAT", "MORPH X", "MORPH Y", "MIX", "DENS" }), Segments ({ "HEAT", "MORPH X", "MORPH Y", "MIX", "DENS" }) };
    ParamRow amt[3] { ParamRow ("QUANTIDADE", kAmber, true), ParamRow ("QUANTIDADE", kAmber, true), ParamRow ("QUANTIDADE", kAmber, true) };
    ParamRow rate { "RATE", kViolet }, atk { "ATTACK", kCyan }, rel { "RELEASE", kCyan }, morph { "A → B", kCoral };
    Segments shape { { "SINE", "TRI", "SQR", "RND" } };
};

// ------------------------------------------------------------------------------------------- janela de ferramenta
class ToolOverlay : public juce::Component
{
public:
    ToolOverlay() { stylePill (closeB); closeB.setButtonText (T8 ("✕")); closeB.onClick = [this] { setVisible (false); }; addAndMakeVisible (closeB); }
    juce::Rectangle<int> card() const { return getLocalBounds().withSizeKeepingCentre (760, 520); }
    void resized() override { closeB.setBounds (card().getRight() - 52, card().getY() + 16, 36, 36); layout(); }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff06070f).withAlpha (0.72f));
        auto c = card().toFloat(); g.setColour (kDisc); g.fillRoundedRectangle (c, 16.f); g.setColour (kLine2); g.drawRoundedRectangle (c.reduced (0.5f), 16.f, 1.f);
        g.setColour (kText); g.setFont (font (20.f, true)); g.drawText (title, card().getX() + 26, card().getY() + 22, 600, 26, juce::Justification::left);
        g.setColour (kDim); g.setFont (font (12.f)); g.drawFittedText (subtitle, card().getX() + 26, card().getY() + 52, card().getWidth() - 100, 34, juce::Justification::topLeft, 2, 1.0f);
        body (g, card().reduced (26).withTrimmedTop (74));
    }
    bool keyPressed (const juce::KeyPress& k) override { if (k == juce::KeyPress::escapeKey) { setVisible (false); return true; } return false; }
protected:
    virtual void body (juce::Graphics&, juce::Rectangle<int>) {}
    virtual void layout() {}
    juce::String title, subtitle; juce::TextButton closeB;
};

// ---- BURNIX Link: todas as instancias da sessao e quanta saturacao cada uma acrescenta
class LinkView : public ToolOverlay
{
public:
    explicit LinkView (BurnixProcessor& pr) : p (pr) {}
    void open() { title = "BURNIX LINK"; subtitle = L ("Quanta saturação cada BURNIX desta sessão está a acrescentar (HARM, dB RMS). Evita empastar a mistura.",
                                                      "How much saturation each BURNIX in this session adds (HARM, dB RMS). Avoid a smeared mix."); setVisible (true); toFront (true); repaint(); }
    void body (juce::Graphics& g, juce::Rectangle<int> r) override
    {
        int y = r.getY(); double sum = 0; int count = 0;
        for (int i = 0; i < BurnixLink::kMax; ++i)
        {
            auto& e = BurnixLink::entries()[(size_t) i]; if (! e.used.load()) continue;
            const float h = e.harm.load(); ++count; if (h > -100) sum += std::pow (10.0, h / 10.0);
            if (y > r.getBottom() - 70) continue;
            juce::String name = juce::String (juce::CharPointer_UTF8 (e.name)); if (name.isEmpty()) name = "BURNIX " + juce::String (count);
            const bool me = i == p.linkSlot();
            g.setColour (me ? kAmber : kText); g.setFont (font (13.f, me)); g.drawText (name + (me ? L ("  (este)", "  (this one)") : juce::String()), r.getX(), y, 280, 22, juce::Justification::centredLeft);
            const float f = juce::jlimit (0.f, 1.f, (h + 60.f) / 60.f);
            g.setColour (kLine); g.fillRoundedRectangle ((float) r.getX() + 290, (float) y + 7, 300, 8, 4); g.setColour (me ? kAmber : kCoral); g.fillRoundedRectangle ((float) r.getX() + 290, (float) y + 7, 300 * f, 8, 4);
            g.setColour (kDim); g.setFont (mono (12.f)); g.drawText (h <= -119 ? juce::String ("--") : juce::String (h, 1) + " dB", r.getRight() - 100, y, 100, 22, juce::Justification::centredRight);
            y += 28;
        }
        g.setColour (kDim); g.setFont (font (12.f));
        g.drawFittedText (juce::String (count) + L (" instâncias ativas · saturação acumulada ", " active instances · accumulated saturation ") + (sum > 0 ? juce::String (10 * std::log10 (sum), 1) + " dB" : juce::String ("--"))
                          + L (".  Só aparecem as instâncias que a DAW carrega no mesmo processo (Studio One, Logic, Reaper, Live...). O nome da pista aparece quando a DAW o envia.",
                               ".  Only instances the DAW loads in the same process are listed (Studio One, Logic, Reaper, Live...). The track name shows when the DAW provides it."),
                          r.removeFromBottom (48), juce::Justification::bottomLeft, 3, 1.0f);
    }
private:
    BurnixProcessor& p;
};

// ---- ASSISTENTE: ouve 10 s da entrada, diz o que parece ser e sugere presets
class AssistantView : public ToolOverlay
{
public:
    explicit AssistantView (BurnixProcessor& pr) : p (pr)
    {
        stylePill (listen, kAmber, kBg); listen.setToggleState (true, juce::dontSendNotification); listen.onClick = [this] { p.startAnalysis (10.0); started = true; repaint(); };
        addAndMakeVisible (listen);
        for (int i = 0; i < 3; ++i) { auto* b = loadB.add (new juce::TextButton()); stylePill (*b); b->onClick = [this, i] { if (sugg[(size_t) i] >= 0) { p.applyPreset (sugg[(size_t) i]); if (onLoaded) onLoaded(); setVisible (false); } }; addChildComponent (b); }
    }
    std::function<void()> onLoaded;
    void open()
    {
        title = L ("ASSISTENTE", "ASSISTANT");
        subtitle = L ("Toca a pista e carrega em OUVIR: o BURNIX ouve 10 segundos da entrada, diz o que lhe parece e sugere presets. Regras simples e explicadas, sem IA na nuvem.",
                      "Play the track and press LISTEN: BURNIX listens to 10 seconds of the input, says what it sounds like and suggests presets. Simple, explained rules, no cloud AI.");
        listen.setButtonText (L ("OUVIR 10 s", "LISTEN 10 s")); started = false; for (auto* b : loadB) b->setVisible (false);
        setVisible (true); toFront (true); resized(); repaint();
    }
    void layout() override
    {
        auto c = card().reduced (26); listen.setBounds (c.getX(), c.getY() + 78, 160, 34);
        for (int i = 0; i < loadB.size(); ++i) loadB[i]->setBounds (c.getX() + i * 238, c.getBottom() - 40, 228, 38);
    }
    void tick() { if (isVisible()) { if (started && p.analysisDone() && ! loadB[0]->isVisible()) suggest(); repaint(); } }
    void body (juce::Graphics& g, juce::Rectangle<int> r) override
    {
        r.removeFromTop (48);
        if (! started) { g.setColour (kDim); g.setFont (font (13.f)); g.drawText (L ("À espera...", "Waiting..."), r.removeFromTop (24), juce::Justification::left); return; }
        if (! p.analysisDone())
        {
            const float f = p.analysisProgress();
            g.setColour (kLine); g.fillRoundedRectangle ((float) r.getX(), (float) r.getY() + 8, (float) r.getWidth(), 8, 4); g.setColour (kAmber); g.fillRoundedRectangle ((float) r.getX(), (float) r.getY() + 8, r.getWidth() * f, 8, 4);
            g.setColour (kDim); g.setFont (font (12.f)); g.drawText (L ("A ouvir... (a DAW tem de estar a tocar)", "Listening... (the DAW must be playing)"), r.getX(), r.getY() + 24, 400, 18, juce::Justification::left);
            return;
        }
        const auto a = p.analysisResult();
        if (! a.valid) { g.setColour (kAmber); g.setFont (font (13.f)); g.drawText (L ("Não chegou som suficiente. Toca a pista e tenta outra vez.", "Not enough sound arrived. Play the track and try again."), r.removeFromTop (24), juce::Justification::left); return; }
        const auto gs = bx::classify (a);
        g.setColour (kText); g.setFont (font (22.f, true)); g.drawText (L ("Parece: ", "Sounds like: ") + catName (gs.category), r.removeFromTop (30), juce::Justification::left);
        g.setColour (kDim); g.setFont (font (13.f)); g.drawText (L ("Porquê: ", "Why: ") + juce::String (juce::CharPointer_UTF8 (gLang == 1 ? gs.reasonEn : gs.reasonPt)), r.removeFromTop (22), juce::Justification::left);
        r.removeFromTop (10); g.setFont (mono (12.f));
        const juce::String lines[4] {
            L ("Energia abaixo de 200 Hz: ", "Energy below 200 Hz: ") + juce::String (juce::roundToInt (a.lowShare * 100)) + " %",
            L ("Centro do espectro: ", "Spectral centre: ") + juce::String (juce::roundToInt (a.centroidHz)) + " Hz",
            L ("Correlação estéreo: ", "Stereo correlation: ") + juce::String (a.correlation, 2),
            L ("Ataques por segundo: ", "Attacks per second: ") + juce::String (a.attacksPerSec, 1) };
        for (auto& l : lines) g.drawText (l, r.removeFromTop (20), juce::Justification::left);
        r.removeFromTop (12); g.setColour (kDim); g.setFont (font (10.f, true)); g.drawText (L ("PRESETS SUGERIDOS", "SUGGESTED PRESETS"), r.removeFromTop (16), juce::Justification::left);
    }
private:
    void suggest()
    {
        const auto gs = bx::classify (p.analysisResult()); const auto& l = bxp::factoryPresets(); int k = 0;
        for (int i = 0; i < (int) l.size() && k < 3; ++i) if (juce::String (l[(size_t) i].category) == gs.category) { sugg[(size_t) k] = i; loadB[k]->setButtonText (l[(size_t) i].name); loadB[k]->setVisible (true); ++k; }
        for (; k < 3; ++k) sugg[(size_t) k] = -1;
    }
    static juce::String catName (const juce::String& c)
    {
        static const char* cat[][3] { { "Voz", "Voz", "Vocals" }, { "Baixo", "Baixo", "Bass" }, { "808 & Sub", "808 & Sub", "808 & Sub" }, { "Kick", "Kick", "Kick" },
            { "Bateria", "Bateria", "Drums" }, { "Percussao", "Percuss\xc3\xa3o", "Percussion" }, { "Guitarra", "Guitarra / teclas", "Guitar / keys" }, { "Mix Bus", "Mix Bus", "Mix Bus" }, { "Master", "Master", "Master" } };
        for (auto& e : cat) if (c == e[0]) return L (e[1], e[2]);
        return c;
    }
    BurnixProcessor& p; juce::TextButton listen; juce::OwnedArray<juce::TextButton> loadB; std::array<int, 3> sugg { -1, -1, -1 }; bool started = false;
};

// ---- MATCH DE REFERENCIA: crista por banda da referencia contra a da entrada -> drive por banda
class MatchView : public ToolOverlay
{
public:
    explicit MatchView (BurnixProcessor& pr) : p (pr)
    {
        stylePill (pick, kAmber, kBg); pick.setToggleState (true, juce::dontSendNotification); stylePill (apply, kCyan, kBg); apply.setToggleState (true, juce::dontSendNotification);
        pick.onClick = [this] { choose(); }; apply.onClick = [this] { doApply(); };
        addAndMakeVisible (pick); addChildComponent (apply);
    }
    std::function<void()> onApplied;
    void open()
    {
        title = L ("MATCH DE REFERÊNCIA", "REFERENCE MATCH");
        subtitle = L ("Escolhe uma faixa de referência (WAV/AIFF/FLAC) e toca a tua pista: o BURNIX compara a densidade (fator de crista) banda a banda e propõe o drive.",
                      "Pick a reference track (WAV/AIFF/FLAC) and play your track: BURNIX compares density (crest factor) band by band and proposes the drive.");
        pick.setButtonText (L ("ESCOLHER REFERÊNCIA...", "CHOOSE REFERENCE...")); apply.setButtonText (L ("APLICAR DRIVES", "APPLY DRIVES"));
        setVisible (true); toFront (true); resized(); repaint();
    }
    void layout() override { auto c = card().reduced (26); pick.setBounds (c.getX(), c.getY() + 78, 220, 34); apply.setBounds (c.getRight() - 200, c.getBottom() - 40, 200, 38); }
    void tick() { if (isVisible()) { if (refDone.load() && p.analysisDone() && ! apply.isVisible()) compute(); repaint(); } }
    void body (juce::Graphics& g, juce::Rectangle<int> r) override
    {
        r.removeFromTop (48); g.setFont (font (12.f));
        g.setColour (kDim); g.drawText (refName.isEmpty() ? L ("Sem referência.", "No reference.") : L ("Referência: ", "Reference: ") + refName + (refDone.load() ? L ("  (analisada)", "  (analysed)") : L ("  (a analisar...)", "  (analysing...)")), r.removeFromTop (20), juce::Justification::left);
        if (refName.isNotEmpty())
            g.drawText (p.analysisDone() ? L ("A tua entrada: analisada", "Your input: analysed") : L ("A tua entrada: a ouvir... ", "Your input: listening... ") + juce::String (juce::roundToInt (p.analysisProgress() * 100)) + " %  " + L ("(a DAW tem de estar a tocar)", "(the DAW must be playing)"),
                        r.removeFromTop (20), juce::Justification::left);
        r.removeFromTop (12);
        if (! apply.isVisible()) return;
        g.setColour (kDim); g.setFont (font (10.f, true));
        g.drawText (L ("BANDA", "BAND"), r.getX(), r.getY(), 200, 14, juce::Justification::left); g.drawText (L ("CRISTA REF.", "REF. CREST"), r.getX() + 230, r.getY(), 120, 14, juce::Justification::left);
        g.drawText (L ("CRISTA TUA", "YOUR CREST"), r.getX() + 360, r.getY(), 120, 14, juce::Justification::left); g.drawText (L ("DRIVE", "DRIVE"), r.getX() + 500, r.getY(), 120, 14, juce::Justification::left);
        r.removeFromTop (20); g.setFont (mono (12.f));
        for (int b = 0; b < nb; ++b)
        {
            auto row = r.removeFromTop (24); g.setColour (bandColour (b));
            g.drawText (L ("Banda ", "Band ") + juce::String (b + 1) + "  " + rng[(size_t) b], row.getX(), row.getY(), 220, 22, juce::Justification::left);
            g.setColour (kText); g.drawText (juce::String (refC[(size_t) b], 1) + " dB", row.getX() + 230, row.getY(), 120, 22, juce::Justification::left);
            g.drawText (juce::String (inC[(size_t) b], 1) + " dB", row.getX() + 360, row.getY(), 120, 22, juce::Justification::left);
            g.setColour (delta[(size_t) b] > 0.05 ? kCoral : (delta[(size_t) b] < -0.05 ? kCyan : kDim));
            g.drawText ((delta[(size_t) b] >= 0 ? "+" : "") + juce::String (delta[(size_t) b], 1), row.getX() + 500, row.getY(), 120, 22, juce::Justification::left);
        }
        g.setColour (kDim); g.setFont (font (11.f));
        g.drawFittedText (L ("Crista mais alta na tua entrada = menos densa do que a referência = mais drive nessa banda. É um ponto de partida: confirma de ouvido.",
                             "Higher crest on your input = less dense than the reference = more drive on that band. A starting point: confirm by ear."), r.removeFromTop (40), juce::Justification::topLeft, 2);
    }
private:
    void choose()
    {
        chooser = std::make_unique<juce::FileChooser> (L ("Faixa de referência", "Reference track"), juce::File(), "*.wav;*.aif;*.aiff;*.flac");
        juce::Component::SafePointer<MatchView> sp (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [sp] (const juce::FileChooser& fc)
        {
            if (sp == nullptr) return; const auto f = fc.getResult(); if (! f.existsAsFile()) return;
            sp->refName = f.getFileName(); sp->refDone = false; sp->apply.setVisible (false); sp->p.startAnalysis (10.0);
            juce::Thread::launch ([sp, f]
            {
                juce::AudioFormatManager fm; fm.registerBasicFormats();
                std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f)); if (rd == nullptr) return;
                bx::Analyzer an; an.prepare (rd->sampleRate);
                const juce::int64 len = std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 90));
                juce::AudioBuffer<float> b (2, 8192);
                for (juce::int64 pos = 0; pos < len; pos += 8192)
                {
                    const int n = (int) std::min<juce::int64> (8192, len - pos); rd->read (&b, 0, n, pos, true, true);
                    for (int i = 0; i < n; ++i) an.push (b.getSample (0, i), b.getSample (rd->numChannels > 1 ? 1 : 0, i));
                }
                const auto res = an.result();
                juce::MessageManager::callAsync ([sp, res] { if (sp != nullptr) { sp->refRes = res; sp->refDone = true; } });
            });
        });
    }
    // crista media (ponderada pela sobreposicao em oitavas) das bandas do analisador dentro de [lo, hi]
    static double crestIn (const bx::Analyzer::Result& r, double lo, double hi)
    {
        double s = 0, w = 0;
        for (int k = 0; k < bx::Analyzer::kBands; ++k)
        {
            const double a = std::max (lo, bx::Analyzer::kEdges[k]), b = std::min (hi, bx::Analyzer::kEdges[k + 1]);
            if (b > a) { const double ww = std::log2 (b / a); s += ww * r.crestDb[(size_t) k]; w += ww; }
        }
        return w > 0 ? s / w : 0;
    }
    void compute()
    {
        const auto e = p.makeEngineParams(); nb = e.numBands; const auto in = p.analysisResult();
        if (! in.valid || ! refRes.valid) return;
        double lo = 20;
        for (int b = 0; b < nb; ++b)
        {
            const double hi = b < nb - 1 ? juce::jlimit (lo * 1.26, 20000.0 / 1.26, e.xo[b]) : 20000.0;
            refC[(size_t) b] = crestIn (refRes, lo, hi); inC[(size_t) b] = crestIn (in, lo, hi);
            delta[(size_t) b] = juce::jlimit (-4.0, 4.0, 0.8 * (inC[(size_t) b] - refC[(size_t) b]));
            auto hz = [] (double v) { return v >= 1000 ? juce::String (v / 1000.0, 1) + "k" : juce::String (juce::roundToInt (v)); };
            rng[(size_t) b] = hz (lo) + T8 ("–") + hz (hi) + " Hz"; lo = hi;
        }
        apply.setVisible (true);
    }
    void doApply()
    {
        p.undo.beginNewTransaction ("Match");
        for (int b = 0; b < nb; ++b)
            for (const char* id : { "driveM", "driveS" })
                if (auto* prm = p.apvts.getParameter (bxp::bandId (b, id)))
                { const float v = juce::jlimit (0.f, 10.f, prm->convertFrom0to1 (prm->getValue()) + (float) delta[(size_t) b]); prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); prm->endChangeGesture(); }
        if (onApplied) onApplied(); setVisible (false);
    }
    BurnixProcessor& p; juce::TextButton pick, apply; std::unique_ptr<juce::FileChooser> chooser;
    juce::String refName; std::atomic<bool> refDone { false }; bx::Analyzer::Result refRes; int nb = 0;
    std::array<double, bx::kMaxBands> refC {}, inC {}, delta {}; std::array<juce::String, bx::kMaxBands> rng;
};
} // namespace bxui
