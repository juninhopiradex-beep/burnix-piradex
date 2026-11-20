#include "PluginEditor.h"
#include "../Plugin/AppPaths.h"

using namespace bxui;

// ------------------------------------------------------------------------------------------------ login
LoginOverlay::LoginOverlay (std::function<void()> ok) : onOk (std::move (ok))
{
    pw.setPasswordCharacter ((juce::juce_wchar) 0x2022); pw.setJustification (juce::Justification::centred);
    pw.setFont (font (16.f)); pw.onReturnKey = [this] { submit(); };
    remember.setToggleState (true, juce::dontSendNotification);
    stylePill (enter, kCoral, kBg); enter.setToggleState (true, juce::dontSendNotification); enter.onClick = [this] { submit(); };
    relabel();
    msg.setJustificationType (juce::Justification::centred); msg.setFont (font (12.f)); msg.setColour (juce::Label::textColourId, kCoral);
    addAndMakeVisible (pw); addAndMakeVisible (remember); addAndMakeVisible (enter); addAndMakeVisible (msg);
}
void LoginOverlay::relabel()
{
    pw.setTextToShowWhenEmpty (L ("Senha de acesso", "Access password"), kFaint);
    remember.setButtonText (L ("Lembrar neste computador", "Remember on this computer"));
    enter.setButtonText (L ("ENTRAR", "SIGN IN")); msg.setText ({}, juce::dontSendNotification); repaint();
}
void LoginOverlay::resized()
{
    auto r = getLocalBounds().withSizeKeepingCentre (400, 350).reduced (34, 0);
    pw.setBounds (r.getX(), r.getY() + 150, r.getWidth(), 40); remember.setBounds (r.getX(), r.getY() + 198, r.getWidth(), 26);
    enter.setBounds (r.getX(), r.getY() + 234, r.getWidth(), 40); msg.setBounds (r.getX() - 20, r.getY() + 280, r.getWidth() + 40, 34);
}
void LoginOverlay::paint (juce::Graphics& g)
{
    g.fillAll (kBg.withAlpha (0.96f));
    auto card = getLocalBounds().withSizeKeepingCentre (400, 350).toFloat();
    g.setColour (kDisc); g.fillRoundedRectangle (card, 20.f); g.setColour (kLine2); g.drawRoundedRectangle (card.reduced (0.5f), 20.f, 1.f);
    // anel de assinatura
    const auto c = card.getCentre().withY (card.getY() + 58);
    juce::Path ring; ring.addCentredArc (c.x, c.y, 26, 26, 0, juce::degreesToRadians (-135.f), juce::degreesToRadians (135.f), true);
    g.setGradientFill (juce::ColourGradient (kCyan, c.x - 26, c.y, kViolet, c.x + 26, c.y, false)); g.strokePath (ring, juce::PathStrokeType (5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (kCoral); g.fillEllipse (c.x - 6, c.y - 6, 12, 12);
    drawLogo (g, card.getCentreX(), card.getY() + 118.f, 26.f, true, false);
    g.setColour (kDim); g.setFont (font (11.f)); g.drawText (T8 ("by PIRADEX  ·  Harmonic Color Engine"), card.withTrimmedTop (118).withHeight (16).toNearestInt(), juce::Justification::centred);
}
void LoginOverlay::submit()
{
    if (busy) return;
    const juce::String pass = pw.getText();
    if (pass.trim().isEmpty()) { msg.setText (L ("Escreva a senha.", "Type the password."), juce::dontSendNotification); return; }
    busy = true; enter.setEnabled (false); msg.setColour (juce::Label::textColourId, kDim); msg.setText (L ("A verificar...", "Checking..."), juce::dontSendNotification);
    const bool rem = remember.getToggleState(); juce::Component::SafePointer<LoginOverlay> safe (this);
    juce::Thread::launch ([safe, pass, rem]
    {
        const auto m = AccessManager::matchPassword (pass);                 // PBKDF2 fora da thread de mensagens
        AccessManager::PccOutcome po = AccessManager::PccOutcome::Disabled; juce::String pmsg;
        if (m.index < 0 && ! m.expired && AccessManager::get().pccConfigured()) po = AccessManager::get().loginPcc (pass, rem, pmsg);
        juce::MessageManager::callAsync ([safe, m, rem, po, pmsg]
        {
            if (safe == nullptr) return;
            safe->busy = false; safe->enter.setEnabled (true); safe->msg.setColour (juce::Label::textColourId, kCoral);
            if (po == AccessManager::PccOutcome::Ok) { safe->pw.clear(); safe->msg.setText ({}, juce::dontSendNotification); if (safe->onOk) safe->onOk(); return; }
            if (po == AccessManager::PccOutcome::Unreachable) { safe->msg.setText (L ("Sem ligação ao servidor de licenças. Verifique a internet.", "No connection to the licence server. Check the internet."), juce::dontSendNotification); return; }
            if (po == AccessManager::PccOutcome::BadReply)    { safe->msg.setText (L ("Resposta do servidor não reconhecida.", "Unrecognised server reply."), juce::dontSendNotification); return; }
            if (po == AccessManager::PccOutcome::Invalid)     { safe->msg.setText (pmsg.isNotEmpty() ? pmsg : L ("Licença inválida, expirada ou desativada.", "Licence invalid, expired or disabled."), juce::dontSendNotification); return; }
            const auto res = AccessManager::get().applyLogin (m, rem);
            if (res == AccessManager::LoginResult::Ok) { safe->pw.clear(); safe->msg.setText ({}, juce::dontSendNotification); if (safe->onOk) safe->onOk(); }
            else if (res == AccessManager::LoginResult::Expired) safe->msg.setText (L ("Esta senha beta já expirou.", "This beta password has expired."), juce::dontSendNotification);
            else safe->msg.setText (L ("Senha incorreta.", "Wrong password."), juce::dontSendNotification);
        });
    });
}

// ------------------------------------------------------------------------------------------------ editor
BurnixEditor::BurnixEditor (BurnixProcessor& p)
    : AudioProcessorEditor (p), proc (p), orbitView (p), perform (p), sculpt (p), engineer (p),
      login ([this] { login.setVisible (false); })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    for (auto* c : std::initializer_list<juce::Component*> { &tabs, &prev, &presetB, &next, &abB, &undoB, &redoB, &qualityB, &bypass, &gear, &orbitView, &perform, &sculpt, &engineer, &login })
        content.addAndMakeVisible (c);
    tabs.onPick = [this] (int v) { setView (v); };
    stylePill (presetB); stylePill (abB, kText, kBg); stylePill (qualityB, kCyan, kBg); qualityB.setToggleState (true, juce::dontSendNotification);
    presetB.onClick = [this] { presetMenu(); };
    prev.onClick = [this] { const int n = proc.getNumPrograms(); proc.applyPreset ((proc.getCurrentProgram() + n - 1) % n); updateHeader(); };
    next.onClick = [this] { const int n = proc.getNumPrograms(); proc.applyPreset ((proc.getCurrentProgram() + 1) % n); updateHeader(); };
    abB.onClick = [this] { proc.toggleAB(); updateHeader(); };
    undoB.onClick = [this] { proc.undo.undo(); }; redoB.onClick = [this] { proc.undo.redo(); };
    qualityB.onClick = [this] { qualityMenu(); };
    gear.onClick = [this] { settingsMenu(); };
    bypass.attach (proc.apvts, "bypass");
    orbitView.getBand = [this] { return band; };
    orbitView.setBand = [this] (int b) { setBand (b); };
    sculpt.onBand = [this] (int b) { setBand (b); };
    engineer.onMeasure = [this] { runMeasure (false); };
    engineer.onKeepRef = [this] { runMeasure (true); };
    engineer.report = &report;

    for (int i = 0; i < 2048; ++i) window[(size_t) i] = 0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * i / 2047.f);

    login.setVisible (! AccessManager::get().isUnlocked());
    juce::Thread::launch ([] { AccessManager::get().revalidatePcc(); });

    constrainer.setFixedAspectRatio (1200.0 / 740.0); constrainer.setSizeLimits (900, 555, 2400, 1480);
    setConstrainer (&constrainer); setResizable (true, true);
    view = juce::jlimit (0, 2, proc.uiView); band = juce::jlimit (0, bx::kMaxBands - 1, proc.uiBand - 1);
    setView (view); setBand (band);
    setScale (juce::jlimit (0.75f, 2.0f, proc.uiScale));
    showSpectrum = AccessManager::get().uiInt ("spectrum", 1) != 0;
    setTips (AccessManager::get().uiInt ("tips", 1) != 0);
    applyLanguage (gLang);
    updateHeader();
    setFps (AccessManager::get().uiInt ("fps", 30));
}

void BurnixEditor::setFps (int f) { fps = juce::jlimit (15, 60, f); startTimerHz (fps); }
void BurnixEditor::setTips (bool on) { if (on && tips == nullptr) tips = std::make_unique<juce::TooltipWindow> (this, 600); else if (! on) tips.reset(); }

void BurnixEditor::applyLanguage (int lang)
{
    gLang = lang == 1 ? 1 : 0;
    prev.setTooltip (L ("Preset anterior", "Previous preset")); next.setTooltip (L ("Preset seguinte", "Next preset"));
    undoB.setTooltip (L ("Desfazer", "Undo")); redoB.setTooltip (L ("Refazer", "Redo"));
    gear.setTooltip (L ("Definições: idioma, cores, escala, animação, predefinições, sessão", "Settings: language, colours, scale, animation, defaults, session"));
    abB.setTooltip (L ("Comparar duas regulações (A/B). Ligue o AUTO GAIN para comparar ao mesmo volume.", "Compare two settings (A/B). Turn on AUTO GAIN to compare at equal loudness."));
    bypass.setTooltip (L ("Bypass real: a entrada passa intacta, alinhada pela latência.", "True bypass: the input passes untouched, latency-aligned."));
    qualityB.setTooltip (L ("Qualidade do oversampling", "Oversampling quality"));
    perform.relabel(); sculpt.relabel(); engineer.relabel(); login.relabel();
    content.repaint(); orbitView.repaint();
}

void BurnixEditor::applySkinChoice (int sk)
{
    applySkin (sk); lnf.refresh(); restyleTree (content);
    for (auto* c : std::initializer_list<juce::Component*> { &perform, &sculpt, &engineer }) restyleTree (*c);
    content.repaint();
}

BurnixEditor::~BurnixEditor() { stopTimer(); setLookAndFeel (nullptr); }

void BurnixEditor::setScale (float s)
{
    scale = juce::jlimit (0.75f, 2.0f, s); proc.uiScale = scale;
    setSize (juce::roundToInt (1200 * scale), juce::roundToInt (740 * scale));
}

void BurnixEditor::resized()
{
    scale = getWidth() / 1200.f; proc.uiScale = scale;
    content.setBounds (0, 0, 1200, 740); content.setTransform (juce::AffineTransform::scale (scale));
    // cabecalho
    tabs.setBounds (300, 17, 330, 30);
    prev.setBounds (650, 17, 30, 30); presetB.setBounds (684, 17, 180, 30); next.setBounds (868, 17, 30, 30);
    abB.setBounds (906, 17, 40, 30); undoB.setBounds (952, 17, 30, 30); redoB.setBounds (986, 17, 30, 30);
    qualityB.setBounds (1022, 17, 64, 30); bypass.setBounds (1092, 17, 66, 30); gear.setBounds (1162, 17, 30, 30);
    orbitView.setBounds (0, 64, 780, 676);
    for (auto* pnl : std::initializer_list<juce::Component*> { &perform, &sculpt, &engineer }) pnl->setBounds (800, 86, 372, 632);
    login.setBounds (0, 0, 1200, 740);
}

void BurnixEditor::setView (int v)
{
    view = juce::jlimit (0, 2, v); proc.uiView = view;
    tabs.setSelected (view); orbitView.view = view;
    perform.setVisible (view == 0); sculpt.setVisible (view == 1); engineer.setVisible (view == 2);
    if (view == 2 && ! report.valid) runMeasure (false);
    orbitView.repaint();
}

void BurnixEditor::setBand (int b)
{
    const int nb = (int) proc.apvts.getRawParameterValue ("numBands")->load();
    band = juce::jlimit (0, nb - 1, b); proc.uiBand = band + 1;
    sculpt.setBand (band); orbitView.repaint();
}

void BurnixEditor::updateHeader()
{
    presetB.setButtonText (proc.currentPresetName());
    abB.setButtonText (proc.abSlot() == 0 ? "A" : "B");
    qualityB.setButtonText (bxp::qualityChoices()[(int) std::lround (proc.apvts.getRawParameterValue ("quality")->load())].upToFirstOccurrenceOf (" ", false, false)
                            + " " + juce::String (bxp::qualityFactor ((int) std::lround (proc.apvts.getRawParameterValue ("quality")->load()))) + "x");
    content.repaint (0, 0, 300, 64);
}

void BurnixEditor::presetMenu()
{
    juce::PopupMenu m, fac; juce::StringArray cats;
    const auto& list = bxp::factoryPresets();
    for (auto& pr : list) cats.addIfNotAlreadyThere (pr.category);
    for (auto& c : cats)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < (int) list.size(); ++i) if (c == list[(size_t) i].category) sub.addItem (1 + i, list[(size_t) i].name, true, i == proc.getCurrentProgram());
        static const char* cat[][3] { { "Base", "Base", "Base" }, { "Voz", "Voz", "Vocals" }, { "Rap & Trap", "Rap & Trap", "Rap & Trap" }, { "Coros", "Coros", "Backing vocals" },
            { "Baixo", "Baixo", "Bass" }, { "808 & Sub", "808 & Sub", "808 & Sub" }, { "Kick", "Kick", "Kick" }, { "Snare & Clap", "Snare & Clap", "Snare & Clap" },
            { "Bateria", "Bateria", "Drums" }, { "Percussao", "Percuss\xc3\xa3o", "Percussion" }, { "Guitarra", "Guitarra", "Guitar" }, { "Teclas", "Teclas", "Keys" },
            { "Synths", "Synths", "Synths" }, { "Mix Bus", "Mix Bus", "Mix Bus" }, { "Mistura", "Mistura", "Mixing" }, { "Master", "Master", "Master" },
            { "Estereo", "Est\xc3\xa9reo", "Stereo" }, { "Afro & Angola", "Afro & Angola", "Afro & Angola" }, { "Criativo", "Criativo", "Creative" } };
        juce::String name = c; for (auto& e : cat) if (c == e[0]) name = L (e[1], e[2]);
        fac.addSubMenu (name + " (" + juce::String (sub.getNumItems()) + ")", sub);
    }
    m.addSectionHeader (L ("PRESETS DE FÁBRICA", "FACTORY PRESETS")); m.addSubMenu (L ("Fábrica", "Factory"), fac);
    const auto users = proc.userPresets();
    juce::PopupMenu us; for (int i = 0; i < users.size(); ++i) us.addItem (1000 + i, users[i].getFileNameWithoutExtension());
    m.addSubMenu (L ("Meus presets", "My presets"), us, users.size() > 0);
    m.addSeparator(); m.addItem (2000, L ("Guardar preset...", "Save preset..."));
    m.addItem (2001, L ("Abrir a pasta dos presets", "Open the presets folder"));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetB), [this, users] (int r)
    {
        if (r >= 1 && r < 1000) proc.applyPreset (r - 1);
        else if (r >= 1000 && r < 2000) proc.loadUserPreset (users[r - 1000]);
        else if (r == 2001) bxpaths::userPresetDir().startAsProcess();
        else if (r == 2000)
        {
            auto* w = new juce::AlertWindow (L ("Guardar preset", "Save preset"), L ("Nome do preset:", "Preset name:"), juce::MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", proc.currentPresetName()); w->addButton (L ("Guardar", "Save"), 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton (L ("Cancelar", "Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
            juce::Component::SafePointer<BurnixEditor> safe (this);
            w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int res)
            {
                if (safe != nullptr && res == 1) { const auto n = w->getTextEditorContents ("name").trim(); if (n.isNotEmpty()) safe->proc.saveUserPreset (n); safe->updateHeader(); }
            }), true);
        }
        updateHeader();
    });
}

void BurnixEditor::qualityMenu()
{
    juce::PopupMenu m; const int q = (int) std::lround (proc.apvts.getRawParameterValue ("quality")->load());
    const juce::String desc[] { L ("ECO 1x  (sem oversampling, latência 0)", "ECO 1x  (no oversampling, zero latency)"), L ("HQ 4x  (recomendado)", "HQ 4x  (recommended)"), "HQ 8x  (master)", L ("ULTRA 16x  (render / máxima rejeição de aliasing)", "ULTRA 16x  (render / maximum alias rejection)") };
    for (int i = 0; i < 4; ++i) m.addItem (1 + i, desc[i], true, i == q);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (qualityB), [this] (int r) { if (r > 0) setParam (proc, "quality", (float) (r - 1)); updateHeader(); });
}

void BurnixEditor::settingsMenu()
{
    auto& A = AccessManager::get();
    juce::PopupMenu m, lang, col, sc, anim, defs;
    lang.addItem (100, juce::String (juce::CharPointer_UTF8 ("Portugu\xc3\xaas")), true, gLang == 0); lang.addItem (101, "English", true, gLang == 1);
    for (int i = 0; i < 5; ++i) col.addItem (110 + i, L (skins()[i].pt, skins()[i].en), true, gSkin == i);
    for (int pct : { 75, 100, 125, 150, 175, 200 }) sc.addItem (1000 + pct, juce::String (pct) + " %", true, std::abs (scale * 100 - pct) < 2);
    anim.addItem (130, L ("Suave (60 fps)", "Smooth (60 fps)"), true, fps == 60);
    anim.addItem (131, L ("Normal (30 fps)", "Normal (30 fps)"), true, fps == 30);
    anim.addItem (132, L ("Económica (15 fps · Macs antigos)", "Economy (15 fps · older Macs)"), true, fps == 15);
    defs.addItem (150, L ("Usar estas regulações ao abrir o BURNIX", "Use these settings when BURNIX opens"));
    defs.addItem (151, L ("Voltar à predefinição de fábrica", "Back to factory default"), proc.hasDefault());
    defs.addSeparator();
    defs.addItem (152, L ("Copiar regulações", "Copy settings"));
    defs.addItem (153, L ("Colar regulações", "Paste settings"), juce::SystemClipboard::getTextFromClipboard().contains ("<BURNIX"));
    defs.addItem (154, L ("Repor tudo (Init)", "Reset everything (Init)"));

    m.addSectionHeader (L ("DEFINIÇÕES", "SETTINGS"));
    m.addSubMenu (L ("Idioma", "Language"), lang);
    m.addSubMenu (L ("Cores", "Colours"), col);
    m.addSubMenu (L ("Escala da janela", "Window scale"), sc);
    m.addSubMenu (L ("Animação", "Animation"), anim);
    m.addItem (140, L ("Espectro no anel", "Spectrum on the ring"), true, showSpectrum);
    m.addItem (141, L ("Dicas ao passar o rato", "Hover tips"), true, tips != nullptr);
    m.addSubMenu (L ("Regulações", "Settings data"), defs);

    // ---- SOM (motor 1.3)
    juce::PopupMenu snd, curveM, driftM, renderM;
    auto pv = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    curveM.addItem (161, L ("Progressiva (1.3)", "Progressive (1.3)"), true, pv ("driveCurve") < 0.5f);
    curveM.addItem (162, L ("Clássica (1.2)", "Classic (1.2)"), true, pv ("driveCurve") > 0.5f);
    for (int d : { 0, 15, 30, 50, 100 }) driftM.addItem (170 + d, d == 0 ? L ("Desligado", "Off") : juce::String (d) + " %", true, std::abs (pv ("drift") - d) < 0.5f);
    const juce::String rq[] { L ("Igual ao que se ouve", "Same as playback"), "HQ 8x", "ULTRA 16x" };
    for (int i = 0; i < 3; ++i) renderM.addItem (300 + i, rq[i], true, std::lround (pv ("renderQuality")) == i);
    snd.addItem (160, L ("Ganho compensado (o drive não sobe o volume)", "Gain compensation (drive does not raise the level)"), true, pv ("levelComp") > 0.5f);
    snd.addSubMenu (L ("Curva do drive", "Drive curve"), curveM);
    snd.addSubMenu (L ("Drift analógico", "Analogue drift"), driftM);
    snd.addSubMenu (L ("Qualidade ao exportar", "Render quality"), renderM);
    snd.addItem (163, L ("CLIP 0 dBFS na saída", "Output CLIP 0 dBFS"), true, pv ("clip") > 0.5f);
    if (proc.usesLegacyEngine())
    {
        snd.addSeparator();
        snd.addItem (164, L ("Sessão da 1.2: atualizar para o motor 1.3", "1.2 session: upgrade to the 1.3 engine"));
    }
    m.addSubMenu (L ("Som", "Sound"), snd);
    m.addSeparator();
    auto role = A.roleText();
    if (gLang == 1)
        role = role.replace ("MESTRE", "MASTER").replace (juce::CharPointer_UTF8 ("LICEN\xC3\x87""A"), "LICENCE").replace ("LICENCIADO", "LICENSED")
                   .replace (juce::CharPointer_UTF8 (" at\xC3\xA9 "), " until ");
    m.addItem (1, L ("Sessão: ", "Session: ") + (role.isNotEmpty() ? role : (AccessManager::inTrial() ? L ("período aberto até 31/12/2026", "open period until 31/12/2026") : L ("sem sessão", "not signed in"))), false);
    m.addItem (2, L ("Terminar sessão", "Sign out"), role.isNotEmpty());
    m.addItem (3, L ("Sobre o BURNIX...", "About BURNIX..."));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (gear), [this] (int r)
    {
        auto& A = AccessManager::get();
        if (r == 100 || r == 101) { A.setLanguage (r - 100); applyLanguage (r - 100); }
        else if (r >= 110 && r < 115) { A.setSkin (r - 110); applySkinChoice (r - 110); }
        else if (r >= 1000) setScale ((r - 1000) / 100.f);
        else if (r >= 130 && r <= 132) { const int f = r == 130 ? 60 : (r == 131 ? 30 : 15); A.setUiInt ("fps", f); setFps (f); }
        else if (r == 140) { showSpectrum = ! showSpectrum; A.setUiInt ("spectrum", showSpectrum ? 1 : 0); if (! showSpectrum) orbitView.spec.fill (0.f); }
        else if (r == 141) { const bool on = tips == nullptr; A.setUiInt ("tips", on ? 1 : 0); setTips (on); }
        else if (r == 160) setParam (proc, "levelComp", proc.apvts.getRawParameterValue ("levelComp")->load() > 0.5f ? 0.f : 1.f);
        else if (r == 161 || r == 162) setParam (proc, "driveCurve", (float) (r - 161));
        else if (r == 163) setParam (proc, "clip", proc.apvts.getRawParameterValue ("clip")->load() > 0.5f ? 0.f : 1.f);
        else if (r == 164) proc.upgradeEngine();
        else if (r >= 170 && r <= 270) setParam (proc, "drift", (float) (r - 170));
        else if (r >= 300 && r <= 302) setParam (proc, "renderQuality", (float) (r - 300));
        else if (r == 150) proc.saveAsDefault();
        else if (r == 151) proc.clearDefault();
        else if (r == 152) juce::SystemClipboard::copyTextToClipboard (proc.stateAsText());
        else if (r == 153) proc.loadStateFromText (juce::SystemClipboard::getTextFromClipboard());
        else if (r == 154) proc.applyPreset (0);
        else if (r == 2) { A.logout(); login.setVisible (! A.isUnlocked()); }
        else if (r == 3)
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "BURNIX by PIRADEX " + juce::String (JucePlugin_VersionString),
                L ("Harmonic Color Engine: saturação de válvulas multibanda com Mid/Side.\n\nCriador e Desenvolvedor Ricardo Amorim / Juninho Piradex\nBeatfreak Music · www.beatfreakstudio.com\nSuporte: juninhopiradex@hotmail.com",
                   "Harmonic Color Engine: multiband tube saturation with Mid/Side.\n\nCreated and developed by Ricardo Amorim / Juninho Piradex\nBeatfreak Music · www.beatfreakstudio.com\nSupport: juninhopiradex@hotmail.com"));
        updateHeader();
    });
}

void BurnixEditor::runMeasure (bool asRef)
{
    if (measuring.exchange (true)) return;
    const auto params = proc.makeEngineParams(); juce::Component::SafePointer<BurnixEditor> safe (this);
    juce::Thread::launch ([safe, params, asRef]
    {
        const auto r = BurnixProcessor::measureHarmonics (params);
        juce::MessageManager::callAsync ([safe, r, asRef]
        {
            if (safe == nullptr) return;
            safe->measuring = false;
            if (asRef) safe->ref = r; else safe->report = r;
            if (asRef && ! safe->report.valid) safe->report = r;
            safe->orbitView.report = safe->report; safe->orbitView.ref = safe->ref;
            safe->orbitView.repaint(); safe->engineer.repaint();
        });
    });
}

void BurnixEditor::timerCallback()
{
    ++ticks;
    // espectro de saida -> 130 barras logaritmicas no anel
    if (view != 2 && showSpectrum && proc.popSpectrum (fftBuf.data(), 2048))
    {
        for (int i = 0; i < 2048; ++i) fftBuf[(size_t) i] *= window[(size_t) i];
        std::fill (fftBuf.begin() + 2048, fftBuf.end(), 0.f);
        fft.performFrequencyOnlyForwardTransform (fftBuf.data());
        const double sr = juce::jmax (8000.0, proc.getSampleRate());
        for (int k = 0; k < (int) orbitView.spec.size(); ++k)
        {
            const double f1 = 20 * std::pow (1000.0, k / 130.0), f2 = 20 * std::pow (1000.0, (k + 1) / 130.0);
            int b1 = juce::jlimit (1, 1023, (int) (f1 / sr * 2048)), b2 = juce::jlimit (b1, 1023, (int) (f2 / sr * 2048));
            float m = 0; for (int b = b1; b <= b2; ++b) m = juce::jmax (m, fftBuf[(size_t) b]);
            const float db = juce::Decibels::gainToDecibels (m / 512.f, -100.f), v = juce::jlimit (0.f, 1.f, (db + 84.f) / 78.f);
            auto& s = orbitView.spec[(size_t) k]; s = v > s ? v : s * 0.86f + v * 0.14f;
        }
    }
    orbitView.repaint();
    if (ticks % 3 == 0)
    {
        perform.refresh(); sculpt.refresh(); engineer.refresh(); updateHeader();
        undoB.setEnabled (proc.undo.canUndo()); redoB.setEnabled (proc.undo.canRedo());
        const int nb = (int) proc.apvts.getRawParameterValue ("numBands")->load(); if (band >= nb) setBand (nb - 1);
    }
    if (ticks % 300 == 0) { AccessManager::get().checkExpiry(); login.setVisible (! AccessManager::get().isUnlocked()); }
    if (ticks % 30 == 0) proc.undo.beginNewTransaction();
}
