#include "PluginEditor.h"
#include "../Plugin/AppPaths.h"

using namespace bxui;

// ------------------------------------------------------------------------------------------------ login
LoginOverlay::LoginOverlay (std::function<void()> ok)
    : onOk (std::move (ok)), screen ("BURNIX", kCoral, JucePlugin_VersionString)
{
    screen.onSubmit = [this] (const juce::String& p, bool rem) { submit (p, rem); };
    addAndMakeVisible (screen);
}
void LoginOverlay::relabel() { screen.repaint(); }
void LoginOverlay::resized() { screen.setBounds (getLocalBounds()); }
void LoginOverlay::submit (const juce::String& pass, bool rem)
{
    if (busy || pass.trim().isEmpty()) return;
    busy = true; screen.setBusy (true);
    juce::Component::SafePointer<LoginOverlay> safe (this);
    juce::Thread::launch ([safe, pass, rem]
    {
        const auto m = AccessManager::matchPassword (pass);                 // PBKDF2 fora da thread de mensagens
        AccessManager::PccOutcome po = AccessManager::PccOutcome::Disabled; juce::String pmsg;
        if (m.index < 0 && ! m.expired && AccessManager::get().pccConfigured()) po = AccessManager::get().loginPcc (pass, rem, pmsg);
        juce::MessageManager::callAsync ([safe, m, rem, po, pmsg]
        {
            if (safe == nullptr) return;
            safe->busy = false; safe->screen.setBusy (false);
            auto ok = [safe] { safe->screen.clearField(); safe->screen.setStatus ({}, false); if (safe->onOk) safe->onOk(); };
            auto err = [safe] (const juce::String& t) { safe->screen.setStatus (t, true); };
            if (po == AccessManager::PccOutcome::Ok) { ok(); return; }
            if (po == AccessManager::PccOutcome::Unreachable) { err (L ("Sem ligação ao servidor de licenças. Verifique a internet.", "No connection to the licence server. Check the internet.")); return; }
            if (po == AccessManager::PccOutcome::BadReply)    { err (L ("Resposta do servidor não reconhecida.", "Unrecognised server reply.")); return; }
            if (po == AccessManager::PccOutcome::Invalid)     { err (pmsg.isNotEmpty() ? pmsg : L ("Licença inválida, expirada ou desativada.", "Licence invalid, expired or disabled.")); return; }
            const auto res = AccessManager::get().applyLogin (m, rem);
            if (res == AccessManager::LoginResult::Ok) ok();
            else if (res == AccessManager::LoginResult::Expired) err (L ("Esta senha beta já expirou.", "This beta password has expired."));
            else err (L ("Senha incorreta.", "Wrong password."));
        });
    });
}

// ------------------------------------------------------------------------------------------------ editor
BurnixEditor::BurnixEditor (BurnixProcessor& p)
    : AudioProcessorEditor (p), proc (p), orbitView (p), perform (p), sculpt (p), engineer (p), motion (p), meters (p), engLeft (p), browser (p), linkView (p), assistant (p), matchView (p),
      login ([this] { login.setVisible (false); })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    for (auto* c : std::initializer_list<juce::Component*> { &tabs, &prev, &presetB, &next, &abB, &copyB, &undoB, &redoB, &qualityB, &bypass, &gear, &orbitView, &meters, &engLeft, &desc, &perform, &sculpt, &engineer, &motion, &browser, &tour, &linkView, &assistant, &matchView, &login })
        content.addAndMakeVisible (c);
    tabs.onPick = [this] (int v) { setView (v); };
    stylePill (presetB); stylePill (copyB); stylePill (qualityB, kCyan, kBg); qualityB.setToggleState (true, juce::dontSendNotification);
    presetB.onClick = [this] { if (juce::ModifierKeys::currentModifiers.isPopupMenu()) presetMenu(); else openBrowser(); };
    copyB.onClick = [this] { proc.copyToOther(); updateHeader(); };
    desc.setFont (bxui::font (12.f)); desc.setJustificationType (juce::Justification::centredLeft); desc.setInterceptsMouseClicks (false, false);
    browser.setVisible (false); tour.setVisible (false); linkView.setVisible (false); assistant.setVisible (false); matchView.setVisible (false);
    assistant.onLoaded = [this] { updateHeader(); perform.refresh(); sculpt.refresh(); engineer.refresh(); };
    matchView.onApplied = [this] { orbitView.repaint(); sculpt.refresh(); };
    engLeft.onDrawMode = [this] (bool d) { orbitView.drawMode = d; orbitView.repaint(); };
    browser.onClose = [this] { browser.setVisible (false); };
    browser.onLoaded = [this] { updateHeader(); perform.refresh(); sculpt.refresh(); engineer.refresh(); };
    browser.onMenu = [this] { browser.setVisible (false); presetMenu(); };
    tour.onFinish = [] { AccessManager::get().setUiInt ("tour", 1); };
    engLeft.onMeasure = [this] { runMeasure (false); };
    engLeft.onKeepRef = [this] { runMeasure (true); };
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

    for (int i = 0; i < 2048; ++i) window[(size_t) i] = 0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * i / 2047.f);

    login.setVisible (! AccessManager::get().isUnlocked());
    juce::Thread::launch ([] { AccessManager::get().revalidatePcc(); });

    constrainer.setFixedAspectRatio (1200.0 / 740.0); constrainer.setSizeLimits (900, 555, 2400, 1480);
    setConstrainer (&constrainer); setResizable (true, true);
    view = juce::jlimit (0, 3, proc.uiView); band = juce::jlimit (0, bx::kMaxBands - 1, proc.uiBand - 1);
    setView (view); setBand (band);
    setScale (juce::jlimit (0.75f, 2.0f, proc.uiScale));
    showSpectrum = AccessManager::get().uiInt ("spectrum", 1) != 0;
    setTips (AccessManager::get().uiInt ("tips", 1) != 0);
    applyLanguage (gLang);
    updateHeader();
    setFps (AccessManager::get().uiInt ("fps", 30));
    // 1.0.2: tutorial e novidades nas 3 primeiras aberturas de cada versao (a visita guiada continua no menu)
    if (piradex::kit::Welcome::shouldShow ("burnix", JucePlugin_VersionString)) showWelcome();
}

void BurnixEditor::showWelcome()
{
    using W = piradex::kit::WelcomeItem;
    welcome = std::make_unique<piradex::kit::Welcome> ("BURNIX", kCoral, JucePlugin_VersionString, std::vector<W> {
        W { "O anel", "The ring",
            "Arraste o ponto no anel: \xc3\xa2ngulo = car\xc3\xa1" "cter das v\xc3\xa1lvulas, dist\xc3\xa2ncia ao centro = HEAT. Cada banda tem o seu ponto.",
            "Drag the dot on the ring: angle = tube character, distance from the centre = HEAT. Each band has its own dot." },
        W { "Quatro vistas", "Four views",
            "PERFORM para tocar r\xc3\xa1pido, SCULPT para as bandas e M/S, ENGINEER para medir, MOTION para modula\xc3\xa7\xc3\xa3o.",
            "PERFORM to play fast, SCULPT for bands and M/S, ENGINEER to measure, MOTION for modulation." },
        W { "Qualidade e CPU", "Quality and CPU",
            "Novo modo MIX 2x por omiss\xc3\xa3o (leve); ao exportar o BURNIX passa sozinho para HQ 4x. Em sil\xc3\xaancio o motor descansa.",
            "New MIX 2x default mode (light); on export BURNIX switches to HQ 4x by itself. In silence the engine rests." },
        W { "Novo na 1.0.2", "New in 1.0.2",
            "CLIP 0 dBFS ligado por omiss\xc3\xa3o, crossovers suaves ao automatizar, idioma PT / EN / ZH e ecr\xc3\xa3 de login Piradex unificado.",
            "CLIP 0 dBFS on by default, smooth crossovers when automated, PT / EN / ZH language and the unified Piradex login screen." } });
    welcome->onClose = [safe = juce::Component::SafePointer<BurnixEditor> (this)] { if (safe != nullptr) juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->welcome.reset(); }); };
    content.addAndMakeVisible (*welcome);
    welcome->setBounds (0, 0, 1200, 740);
    welcome->toFront (true);
}

void BurnixEditor::openBrowser() { browser.setVisible (true); browser.toFront (true); browser.open(); }

void BurnixEditor::startTour()
{
    // alvos: o arco MID da banda 2 (ou 1), o ponto do morph e o botao DELTA PURO
    setView (0);
    const auto e = proc.makeEngineParams(); const int b = e.numBands > 1 ? 1 : 0;
    double lo = 20, hi = 20000;
    { double x = 20; for (int k = 0; k < e.numBands - 1; ++k) { x = juce::jlimit (x * 1.26, 20000.0 / 1.26, e.xo[k]); if (k == b - 1) lo = x; if (k == b) hi = x; } }
    const auto arc = bxui::OrbitView::pt (232.f, 0.5f * (bxui::OrbitView::angleOf (lo) + bxui::OrbitView::angleOf (hi))) + juce::Point<float> (0, 64);
    const juce::Point<float> puck (bxui::OrbitView::CX + ((float) e.morphX - 0.5f) * 256.f, bxui::OrbitView::CY + 64 + ((float) e.morphY - 0.5f) * 256.f);
    tour.targets = { juce::Rectangle<float> (130, 130).withCentre (arc), juce::Rectangle<float> (90, 90).withCentre (puck),
                     perform.deltaBounds().toFloat().translated (800, 86).expanded (8, 8) };
    tour.toFront (true); tour.start();
}

void BurnixEditor::startBlind()
{
    // troca o conteudo de A e B ao acaso; X = lado 0, Y = lado 1. So o revelar diz qual era qual.
    blindSwapped = juce::Random::getSystemRandom().nextBool();
    if (blindSwapped) proc.swapAB();
    blind = true; applyLanguage (gLang); updateHeader();
}

void BurnixEditor::revealBlind (bool showResult)
{
    if (! blind) return;
    const int now = proc.abSlot();
    const juce::String xIs = blindSwapped ? "B" : "A", yIs = blindSwapped ? "A" : "B";
    if (blindSwapped) proc.swapAB();                    // devolve A e B aos seus lados
    blind = false; applyLanguage (gLang); updateHeader();
    if (showResult) juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, L ("Teste cego", "Blind test"),
        "X = " + xIs + "   ·   Y = " + yIs + "\n\n" + L ("Estavas a ouvir: ", "You were listening to: ") + (now == 0 ? "X (" + xIs + ")" : "Y (" + yIs + ")"));
}

void BurnixEditor::setFps (int f) { fps = juce::jlimit (15, 60, f); startTimerHz (fps); }
void BurnixEditor::setTips (bool on) { if (on && tips == nullptr) tips = std::make_unique<juce::TooltipWindow> (this, 600); else if (! on) tips.reset(); }

void BurnixEditor::applyLanguage (int lang)
{
    gLang = juce::jlimit (0, 2, lang);
    prev.setTooltip (L ("Preset anterior", "Previous preset")); next.setTooltip (L ("Preset seguinte", "Next preset"));
    undoB.setTooltip (L ("Desfazer", "Undo")); redoB.setTooltip (L ("Refazer", "Redo"));
    gear.setTooltip (L ("Definições: idioma, cores, escala, animação, predefinições, sessão", "Settings: language, colours, scale, animation, defaults, session"));
    abB.setTooltip (L ("Comparar duas regulações (A/B). Com o ganho compensado ligado, a comparação é ao mesmo volume.", "Compare two settings (A/B). With gain compensation on, the comparison is level-matched."));
    copyB.setTooltip (L ("Copiar o lado atual para o outro (A→B)", "Copy the current side to the other (A→B)"));
    presetB.setTooltip (L ("Abrir o navegador de presets (clique direito: guardar e pasta)", "Open the preset browser (right-click: save and folder)"));
    bypass.setTooltip (L ("Bypass real: a entrada passa intacta, alinhada pela latência.", "True bypass: the input passes untouched, latency-aligned."));
    qualityB.setTooltip (L ("Qualidade do oversampling", "Oversampling quality"));
    perform.relabel(); sculpt.relabel(); engineer.relabel(); engLeft.relabel(); browser.relabel(); login.relabel(); motion.relabel(); meters.relabel();
    abB.setTooltip (blind ? L ("Teste cego: X e Y escondem qual é o A e o B. ⚙ › Ferramentas › Revelar.", "Blind test: X and Y hide which is A and B. ⚙ › Tools › Reveal.") : abB.getTooltip());
    content.repaint(); orbitView.repaint();
}

void BurnixEditor::applySkinChoice (int sk)
{
    applySkin (sk); lnf.refresh(); restyleTree (content);
    for (auto* c : std::initializer_list<juce::Component*> { &perform, &sculpt, &engineer, &motion, &engLeft, &browser, &tour, &linkView, &assistant, &matchView }) restyleTree (*c);
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
    tabs.setBounds (276, 17, 312, 30);
    prev.setBounds (600, 17, 30, 30); presetB.setBounds (634, 17, 190, 30); next.setBounds (828, 17, 30, 30);
    abB.setBounds (864, 17, 56, 30); copyB.setBounds (924, 17, 46, 30); undoB.setBounds (976, 17, 30, 30); redoB.setBounds (1008, 17, 30, 30);
    qualityB.setBounds (1044, 17, 50, 30); bypass.setBounds (1098, 17, 60, 30); gear.setBounds (1162, 17, 30, 30);
    orbitView.setBounds (0, 64, 780, 676);
    desc.setBounds (20, 68, 760, 20);
    meters.setBounds (700, 110, 92, 580);
    engLeft.setBounds (24, 548, 740, 182);
    for (auto* o : std::initializer_list<juce::Component*> { &browser, &tour, &linkView, &assistant, &matchView }) o->setBounds (0, 0, 1200, 740);
    if (welcome != nullptr) welcome->setBounds (0, 0, 1200, 740);
    for (auto* pnl : std::initializer_list<juce::Component*> { &perform, &sculpt, &engineer, &motion }) pnl->setBounds (800, 86, 372, 632);
    login.setBounds (0, 0, 1200, 740);
}

void BurnixEditor::setView (int v)
{
    view = juce::jlimit (0, 3, v); proc.uiView = view;
    tabs.setSelected (view); orbitView.view = view == 3 ? 0 : view;
    perform.setVisible (view == 0); sculpt.setVisible (view == 1); engineer.setVisible (view == 2); motion.setVisible (view == 3);
    meters.setVisible (view != 2); engLeft.setVisible (view == 2);
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
    presetB.setButtonText (blind ? L ("TESTE CEGO", "BLIND TEST") : proc.currentPresetName());
    abB.slot = proc.abSlot(); abB.blind = blind; abB.repaint(); copyB.setButtonText (blind ? T8 ("·") : (proc.abSlot() == 0 ? T8 ("A→B") : T8 ("B→A"))); copyB.setEnabled (! blind);
    {
        auto d = proc.presetDescription();
        if (blind) d = L ("Teste cego: compara X e Y de ouvido, ao mesmo volume. ⚙ › Ferramentas › Revelar quando escolheres.", "Blind test: compare X and Y by ear, level-matched. ⚙ › Tools › Reveal when you have chosen.");
        else if (proc.usesLegacyEngine()) d = L ("Sessão da 1.2: motor antigo, som igual ao da 1.2 (⚙ › Som para atualizar). ", "1.2 session: old engine, same sound as 1.2 (⚙ › Sound to upgrade). ") + d;
        if (desc.getText() != d) { desc.setText (d, juce::dontSendNotification); desc.setColour (juce::Label::textColourId, proc.usesLegacyEngine() ? bxui::kAmber : bxui::kDim); }
    }
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
    const juce::String desc[] { L ("ECO 1x  (sem oversampling, latência 0)", "ECO 1x  (no oversampling, zero latency)"), "HQ 4x", "HQ 8x  (master)", L ("ULTRA 16x  (render / máxima rejeição de aliasing)", "ULTRA 16x  (render / maximum alias rejection)") , L ("MIX 2x  (a tocar 2x, exporta em 4x \xe2\x80\x94 recomendado)", "MIX 2x  (2x while playing, renders at 4x \xe2\x80\x94 recommended)")};
    for (int i = 0; i < 5; ++i) m.addItem (1 + i, desc[i], true, i == q);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (qualityB), [this] (int r) { if (r > 0) setParam (proc, "quality", (float) (r - 1)); updateHeader(); });
}

void BurnixEditor::settingsMenu()
{
    auto& A = AccessManager::get();
    juce::PopupMenu m, lang, col, sc, anim, defs;
    lang.addItem (100, juce::String (juce::CharPointer_UTF8 ("Portugu\xc3\xaas")), true, gLang == 0); lang.addItem (101, "English", true, gLang == 1); lang.addItem (102, juce::String (juce::CharPointer_UTF8 ("\xe4\xb8\xad\xe6\x96\x87")), true, gLang == 2);
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
    m.addItem (142, L ("Visita guiada", "Guided tour"));
    m.addItem (143, L ("Tutorial e novidades", "Tutorial and what's new"));
    m.addSubMenu (L ("Regulações", "Settings data"), defs);
    {
        juce::PopupMenu tools;
        tools.addItem (180, blind ? L ("Teste A/B cego: REVELAR", "Blind A/B test: REVEAL") : L ("Teste A/B cego", "Blind A/B test"));
        tools.addItem (181, L ("BURNIX Link (todas as instâncias)", "BURNIX Link (all instances)"));
        tools.addItem (182, L ("Assistente (ouve e sugere presets)", "Assistant (listens and suggests presets)"));
        tools.addItem (183, L ("Match de referência", "Reference match"));
        m.addSubMenu (L ("Ferramentas", "Tools"), tools);
    }

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
    if (gLang != 0)
        role = role.replace ("MESTRE", "MASTER").replace (juce::CharPointer_UTF8 ("LICEN\xC3\x87""A"), "LICENCE").replace ("LICENCIADO", "LICENSED")
                   .replace (juce::CharPointer_UTF8 (" at\xC3\xA9 "), " until ");
    m.addItem (1, L ("Sessão: ", "Session: ") + (role.isNotEmpty() ? role : (AccessManager::inTrial() ? L ("período aberto até 31/12/2026", "open period until 31/12/2026") : L ("sem sessão", "not signed in"))), false);
    m.addItem (2, L ("Terminar sessão", "Sign out"), role.isNotEmpty());
    m.addItem (3, L ("Sobre o BURNIX...", "About BURNIX..."));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (gear), [this] (int r)
    {
        auto& A = AccessManager::get();
        if (r >= 100 && r <= 102) { A.setLanguage (r - 100); applyLanguage (r - 100); }
        if (r == 143) showWelcome();
        else if (r >= 110 && r < 115) { A.setSkin (r - 110); applySkinChoice (r - 110); }
        else if (r >= 1000) setScale ((r - 1000) / 100.f);
        else if (r >= 130 && r <= 132) { const int f = r == 130 ? 60 : (r == 131 ? 30 : 15); A.setUiInt ("fps", f); setFps (f); }
        else if (r == 140) { showSpectrum = ! showSpectrum; A.setUiInt ("spectrum", showSpectrum ? 1 : 0); if (! showSpectrum) orbitView.spec.fill (0.f); }
        else if (r == 141) { const bool on = tips == nullptr; A.setUiInt ("tips", on ? 1 : 0); setTips (on); }
        else if (r == 142) startTour();
        else if (r == 180) { if (blind) revealBlind(); else startBlind(); }
        else if (r == 181) linkView.open();
        else if (r == 182) assistant.open();
        else if (r == 183) matchView.open();
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
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "BURNIX by Piradex " + juce::String (JucePlugin_VersionString),
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
    orbitView.repaint(); if (view != 2) meters.update();
    if (view == 3) motion.repaint();
    assistant.tick(); matchView.tick(); if (linkView.isVisible() && ticks % 6 == 0) linkView.repaint();
    if (ticks % 3 == 0)
    {
        perform.refresh(); sculpt.refresh(); engineer.refresh(); engLeft.refresh(); motion.refresh(); updateHeader();
        undoB.setEnabled (proc.undo.canUndo()); redoB.setEnabled (proc.undo.canRedo());
        const int nb = (int) proc.apvts.getRawParameterValue ("numBands")->load(); if (band >= nb) setBand (nb - 1);
    }
    if (ticks % 300 == 0) { AccessManager::get().checkExpiry(); login.setVisible (! AccessManager::get().isUnlocked()); }
    if (ticks % 30 == 0) proc.undo.beginNewTransaction();
}
