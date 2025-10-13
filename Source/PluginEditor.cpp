#include "PluginEditor.h"
using boomui::loadSkin;
using boomui::setButtonImages;
using boomui::setToggleImages;
#include "EngineDefs.h"
#include "Theme.h"

namespace {
    void updateEngineButtonSkins(boom::Engine e, juce::ImageButton& btn808, juce::ImageButton& btnBass, juce::ImageButton& btnDrums)
    {
        boomui::setButtonImages(btn808, "808Btn");
        boomui::setButtonImages(btnBass, "bassBtn");
        boomui::setButtonImages(btnDrums, "drumsBtn");

        if (e == boom::Engine::e808)  boomui::setButtonImagesSelected(btn808, "808Btn");
        if (e == boom::Engine::Bass)  boomui::setButtonImagesSelected(btnBass, "bassBtn");
        if (e == boom::Engine::Drums) boomui::setButtonImagesSelected(btnDrums, "drumsBtn");
    }
}

// ======= small helper =======
int BoomAudioProcessorEditor::barsFromBox(const juce::ComboBox& b) { return b.getSelectedId() == 2 ? 8 : 4; }

// ================== Editor ==================
BoomAudioProcessorEditor::BoomAudioProcessorEditor(BoomAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p),
    drumGrid(p), pianoRoll(p)
{
    setLookAndFeel(&boomui::LNF());
    setResizable(true, true);
    setSize(783, 714);

    // Engine label + buttons
    logoImg.setImage(loadSkin("logo.png")); addAndMakeVisible(logoImg);
    soundsDopeLbl.setImage(loadSkin("soundsDopeLbl.png")); addAndMakeVisible(soundsDopeLbl);
    engineLblImg.setImage(loadSkin("engineLbl.png")); addAndMakeVisible(engineLblImg);
    setButtonImages(btn808, "808Btn");    addAndMakeVisible(btn808);
    setButtonImages(btnBass, "bassBtn");   addAndMakeVisible(btnBass);
    setButtonImages(btnDrums, "drumsBtn");  addAndMakeVisible(btnDrums);
    btn808.onClick = [this] { setEngine(boom::Engine::e808);  };
    btnBass.onClick = [this] { setEngine(boom::Engine::Bass);  };
    btnDrums.onClick = [this] { setEngine(boom::Engine::Drums); };

    updateEngineButtonSkins((boom::Engine)(int)proc.apvts.getRawParameterValue("engine")->load(),
        btn808, btnBass, btnDrums);

    // Left labels
    scaleLblImg.setImage(loadSkin("scaleLbl.png"));          addAndMakeVisible(scaleLblImg);
    timeSigLblImg.setImage(loadSkin("timeSigLbl.png"));          addAndMakeVisible(timeSigLblImg);
    barsLblImg.setImage(loadSkin("barsLbl.png"));                addAndMakeVisible(barsLblImg);
    humanizeLblImg.setImage(loadSkin("humanizeLbl.png"));        addAndMakeVisible(humanizeLblImg);
    tripletsLblImg.setImage(loadSkin("tripletsLbl.png"));        addAndMakeVisible(tripletsLblImg);
    dottedNotesLblImg.setImage(loadSkin("dottedNotesLbl.png"));  addAndMakeVisible(dottedNotesLblImg);
    restDensityLblImg.setImage(loadSkin("restDensityLbl.png"));  addAndMakeVisible(restDensityLblImg);
    keyLblImg.setImage(loadSkin("keyLbl.png"));                  addAndMakeVisible(keyLblImg);
    octaveLblImg.setImage(loadSkin("octaveLbl.png"));            addAndMakeVisible(octaveLblImg);
    bassSelectorLblImg.setImage(loadSkin("bassSelectorLbl.png"));  addAndMakeVisible(bassSelectorLblImg);
    drumsSelectorLblImg.setImage(loadSkin("drumsSelectorLbl.png")); addAndMakeVisible(drumsSelectorLblImg);
    eightOhEightLblImg.setImage(loadSkin("808BassLbl.png"));     addAndMakeVisible(eightOhEightLblImg);
    styleLblImg.setImage(loadSkin("styleLbl.png"));     addAndMakeVisible(styleLblImg);
    

    // Left controls
    addAndMakeVisible(timeSigBox); timeSigBox.addItemList(boom::timeSigChoices(), 1);
    addAndMakeVisible(barsBox);    barsBox.addItemList(boom::barsChoices(), 1);

    addAndMakeVisible(humanizeTiming);   humanizeTiming.setSliderStyle(juce::Slider::LinearHorizontal);
    humanizeTiming.setRange(0, 100);
    addAndMakeVisible(humanizeVelocity); humanizeVelocity.setSliderStyle(juce::Slider::LinearHorizontal);
    humanizeVelocity.setRange(0, 100);
    addAndMakeVisible(swing);            swing.setSliderStyle(juce::Slider::LinearHorizontal);
    swing.setRange(0, 100);
    addAndMakeVisible(tripletDensity); tripletDensity.setSliderStyle(juce::Slider::LinearHorizontal);
    tripletDensity.setRange(0, 100);
    addAndMakeVisible(dottedDensity);  dottedDensity.setSliderStyle(juce::Slider::LinearHorizontal);
    dottedDensity.setRange(0, 100);
    dottedDensity.setLookAndFeel(&purpleLNF);
    tripletDensity.setLookAndFeel(&purpleLNF);
    boomui::makePercentSlider(dottedDensity);
    boomui::makePercentSlider(tripletDensity);
    dottedDensity.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    tripletDensity.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    humanizeTiming.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    humanizeVelocity.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    swing.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    // Switches as ImageButtons
    addAndMakeVisible(useTriplets); setToggleImages(useTriplets, "checkBoxOffBtn", "checkBoxOnBtn");
    addAndMakeVisible(useDotted);   setToggleImages(useDotted, "checkBoxOffBtn", "checkBoxOnBtn");

    // APVTS attachments
    timeSigAtt = std::make_unique<Attachment>(proc.apvts, "timeSig", timeSigBox);
    barsAtt = std::make_unique<Attachment>(proc.apvts, "bars", barsBox);
    humanizeTimingAtt = std::make_unique<SAttachment>(proc.apvts, "humanizeTiming", humanizeTiming);
    humanizeVelocityAtt = std::make_unique<SAttachment>(proc.apvts, "humanizeVelocity", humanizeVelocity);
    swingAtt = std::make_unique<SAttachment>(proc.apvts, "swing", swing);
    useTripletsAtt = std::make_unique<BAttachment>(proc.apvts, "useTriplets", useTriplets);
    tripletDensityAtt = std::make_unique<SAttachment>(proc.apvts, "tripletDensity", tripletDensity);
    useDottedAtt = std::make_unique<BAttachment>(proc.apvts, "useDotted", useDotted);
    dottedDensityAtt = std::make_unique<SAttachment>(proc.apvts, "dottedDensity", dottedDensity);

    // 808/Bass
    addAndMakeVisible(keyBox);   keyBox.addItemList(boom::keyChoices(), 1);
    addAndMakeVisible(scaleBox); scaleBox.addItemList(boom::scaleChoices(), 1);
    addAndMakeVisible(octaveBox); octaveBox.addItemList(juce::StringArray("-2", "-1", "0", "+1", "+2"), 1);
    addAndMakeVisible(rest808);  rest808.setSliderStyle(juce::Slider::LinearHorizontal);
    rest808.setRange(0, 100); rest808.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    keyAtt = std::make_unique<Attachment>(proc.apvts, "key", keyBox);
    scaleAtt = std::make_unique<Attachment>(proc.apvts, "scale", scaleBox);
    octaveAtt = std::make_unique<Attachment>(proc.apvts, "octave", octaveBox);
    rest808Att = std::make_unique<SAttachment>(proc.apvts, "restDensity808", rest808);

    addAndMakeVisible(bassStyleBox); bassStyleBox.addItemList(boom::styleChoices(), 1);
    bassStyleAtt = std::make_unique<Attachment>(proc.apvts, "bassStyle", bassStyleBox);

    // Drums
    addAndMakeVisible(drumStyleBox); drumStyleBox.addItemList(boom::styleChoices(), 1);
    addAndMakeVisible(restDrums); restDrums.setSliderStyle(juce::Slider::LinearHorizontal);
    restDrums.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    restDrums.setRange(0, 100);
    drumStyleAtt = std::make_unique<Attachment>(proc.apvts, "drumStyle", drumStyleBox);
    restDrumsAtt = std::make_unique<SAttachment>(proc.apvts, "restDensityDrums", restDrums);

    // Center views
    drumGrid.setRows(proc.getDrumRows());
    drumGrid.onToggle = [this](int row, int tick) { toggleDrumCell(row, tick); };
    addAndMakeVisible(drumGrid);
    addAndMakeVisible(pianoRoll);

    // Right action buttons
    setButtonImages(btnAITools, "aiToolsBtn");  addAndMakeVisible(btnAITools);
    setButtonImages(btnRolls, "rollsBtn");    addAndMakeVisible(btnRolls);
    setButtonImages(btnBumppit, "bumppitBtn");  addAndMakeVisible(btnBumppit);
    setButtonImages(btnFlippit, "flippitBtn");  addAndMakeVisible(btnFlippit);
    setButtonImages(diceBtn, "diceBtn");      addAndMakeVisible(diceBtn);

    btnFlippit.onClick = [this]
    {
        auto engine = (boom::Engine)proc.apvts.getRawParameterValue("engine")->load();

        flippit.reset(new FlippitWindow(
            proc,
            [this] { flippit.reset(); },
            [this](int density)
            {
                const auto eng = proc.getEngineSafe();
                const int bars = barsFromBox(barsBox);
                const int seed = (int)proc.apvts.getRawParameterValue("seed")->load();
                if (eng == boom::Engine::Drums) proc.flipDrums(seed, density, bars);
                else                             proc.flipMelodic(seed, density, bars);
                regenerate();
            },
            engine));

        juce::DialogWindow::LaunchOptions o;
        o.content.setOwned(flippit.release());
        o.dialogTitle = "FLIPPIT";
        o.useNativeTitleBar = true;
        o.resizable = false;
        o.componentToCentreAround = this;
        o.launchAsync();
    };

    btnBumppit.onClick = [this]
    {
        auto engine = (boom::Engine)proc.apvts.getRawParameterValue("engine")->load();

        bumppit.reset(new BumppitWindow(
            proc,
            [this] { bumppit.reset(); },
            [this] { proc.bumpDrumRowsUp(); regenerate(); },
            engine));

        juce::DialogWindow::LaunchOptions o;
        o.content.setOwned(bumppit.release());
        o.dialogTitle = "BUMPPIT";
        o.useNativeTitleBar = true;
        o.resizable = false;
        o.componentToCentreAround = this;
        o.launchAsync();
    };

    btnRolls.onClick = [this]
    {
        rolls.reset(new RollsWindow(proc, [this] { rolls.reset(); },
            [this](juce::String style, int bars, int density)
            { juce::ignoreUnused(style, density); proc.setDrumPattern(makeDemoPatternDrums(bars)); regenerate(); }));
        juce::DialogWindow::LaunchOptions o; o.content.setOwned(rolls.release());
        o.dialogTitle = "ROLLS"; o.useNativeTitleBar = true; o.resizable = false; o.componentToCentreAround = this; o.launchAsync();
    };

    btnAITools.onClick = [this]
    {
        juce::DialogWindow::LaunchOptions opts;
        opts.dialogTitle = "AI Tools";
        opts.componentToCentreAround = this;
        opts.useNativeTitleBar = true;
        opts.escapeKeyTriggersCloseButton = true;
        opts.resizable = true;

        opts.content.setOwned(new AIToolsWindow(proc));  // uses default {} onClose
        if (auto* dw = opts.launchAsync())
        {
            dw->setResizable(true, true);
            dw->centreAroundComponent(this, 1000, 720);
            dw->setVisible(true);
        }

        opts.componentToCentreAround = this;
        opts.launchAsync();
    };

    // Bottom bar: Generate + Drag (ImageButtons)
    setButtonImages(btnGenerate, "generateBtn"); addAndMakeVisible(btnGenerate);
    setButtonImages(btnDragMidi, "dragBtn");     addAndMakeVisible(btnDragMidi);
    btnDragMidi.addMouseListener(this, true); // start drag on mouseDown

    // Init engine & layout
    setEngine((boom::Engine)(int)proc.apvts.getRawParameterValue("engine")->load());
    syncVisibility();
    regenerate();
}

void BoomAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(boomtheme::MainBackground());
}

void BoomAudioProcessorEditor::resized()
{
    const float W = 783.f, H = 714.f;
    auto bounds = getLocalBounds();

    auto sx = bounds.getWidth() / W;
    auto sy = bounds.getHeight() / H;
    auto S = [sx, sy](int x, int y, int w, int h)
    {
        return juce::Rectangle<int>(juce::roundToInt(x * sx),
            juce::roundToInt(y * sy),
            juce::roundToInt(w * sx),
            juce::roundToInt(h * sy));
    };

    // Header
    engineLblImg.setBounds(S(241, 10, 300, 40));
    btn808.setBounds(S(232, 50, 100, 52));
    btnBass.setBounds(S(341, 50, 100, 52));
    btnDrums.setBounds(S(451, 50, 100, 52));

    logoImg.setBounds(S(255, 95, 290, 290));


    // Right column
    diceBtn.setBounds(S(723, 15, 50, 50));
    tripletsLblImg.setBounds(S(610, 10, 73, 26));
    useTriplets.setBounds(S(690, 18, 20, 20));
    tripletDensity.setBounds(S(583, 30, 100, 20));
    dottedNotesLblImg.setBounds(S(565, 45, 114, 26));
    dottedDensity.setBounds(S(568, 65, 100, 20));
    useDotted.setBounds(S(685, 50, 20, 20));
    soundsDopeLbl.setBounds(S(15, 15, 100, 49));
    
    // Left Column
    int y = 130;
    const int x = 10;
    const int lblWidth = 100;
    const int ctlWidth = 125;
    const int height = 26;
    const int spacing = 30;

    keyLblImg.setBounds(S(x, y, lblWidth, height));
    keyBox.setBounds(S(x + lblWidth + 5, y, ctlWidth, height));
    y += spacing;

    scaleLblImg.setBounds(S(x, y, lblWidth, height));
    scaleBox.setBounds(S(x + lblWidth + 5, y, ctlWidth, height));
    y += spacing;

    octaveLblImg.setBounds(S(x, y, lblWidth, height));
    octaveBox.setBounds(S(x + lblWidth + 5, y, ctlWidth, height));
    y += spacing;

    timeSigLblImg.setBounds(S(x, y, lblWidth, height));
    timeSigBox.setBounds(S(x + lblWidth + 5, y, ctlWidth, height));
    y += spacing;

    barsLblImg.setBounds(S(x, y, lblWidth, height));
    barsBox.setBounds(S(x + lblWidth + 5, y, ctlWidth, height));
    y += spacing;

    restDensityLblImg.setBounds(S(x, y, lblWidth, height));
    rest808.setBounds(S(x + lblWidth + 5, y, ctlWidth, height));
    restDrums.setBounds(S(x + lblWidth + 5, y, ctlWidth, height));
    y += spacing;

    styleLblImg.setBounds(S(x, y, lblWidth, height));
    bassStyleBox.setBounds(S(x + lblWidth + 5, y, ctlWidth, height));
    drumStyleBox.setBounds(S(x + lblWidth + 5, y, ctlWidth, height));
    y += spacing;

    // Right Column
    int right_x = 550;
    y = 150;
    humanizeLblImg.setBounds(S(right_x, y, 200, 26));
    y += spacing;
    humanizeTiming.setBounds(S(right_x, y, 200, 50));
    y += spacing;
    humanizeVelocity.setBounds(S(right_x, y, 200, 50));
    y += spacing;
    swing.setBounds(S(right_x, y, 200, 50));


    // Buttons
    btnBumppit.setBounds(S(580, 280, 200, 60));
    btnFlippit.setBounds(S(580, 350, 200, 60));
    btnRolls.setBounds(S(40, 350, 200, 60));
    btnAITools.setBounds(S(290, 350, 200, 60));

    // Center views
    auto center = S(40, 430, 703, 200);
    drumGrid.setBounds(center);
    pianoRoll.setBounds(center);

    // Bottom bar
    btnGenerate.setBounds(S(40, 640, 300, 70));
    btnDragMidi.setBounds(S(443, 640, 300, 70));

    syncVisibility();
}

void BoomAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (e.eventComponent == &btnDragMidi || e.originalComponent == &btnDragMidi)
        startExternalMidiDrag();
}

void BoomAudioProcessorEditor::setEngine(boom::Engine e)
{
    proc.apvts.getParameter("engine")->beginChangeGesture();
    dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter("engine"))->operator=((int)e);
    proc.apvts.getParameter("engine")->endChangeGesture();
    syncVisibility();
    resized();
    updateEngineButtonSkins(e, btn808, btnBass, btnDrums);
}

void BoomAudioProcessorEditor::syncVisibility()
{
    auto engine = (boom::Engine)(int)proc.apvts.getRawParameterValue("engine")->load();
    const bool isDrums = engine == boom::Engine::Drums;
    const bool is808 = engine == boom::Engine::e808;
    const bool isBass = engine == boom::Engine::Bass;

    drumGrid.setVisible(isDrums);
    pianoRoll.setVisible(!isDrums);

    // --- Left Column based on user request ---

    // 808: KEY, SCALE, BARS, OCTAVE, Rest Density
    // BASS: KEY, SCALE, BARS, OCTAVE, STYLE, Rest Density
    // DRUMS: TIME SIGNATURE, BARS, STYLE

    keyLblImg.setVisible(is808 || isBass);
    keyBox.setVisible(is808 || isBass);

    scaleLblImg.setVisible(is808 || isBass);
    scaleBox.setVisible(is808 || isBass);

    barsLblImg.setVisible(true); // all engines
    barsBox.setVisible(true); // all engines

    octaveLblImg.setVisible(is808 || isBass);
    octaveBox.setVisible(is808 || isBass);

    styleLblImg.setVisible(isBass || isDrums);
    bassStyleBox.setVisible(isBass);
    drumStyleBox.setVisible(isDrums);

    restDensityLblImg.setVisible(true);
    rest808.setVisible(is808 || isBass);
    restDrums.setVisible(true);

    timeSigLblImg.setVisible(true);
    timeSigBox.setVisible(true);

    // --- Other UI elements from original implementation ---
    // These seem to be independent of the combo box changes.
    tripletDensity.setVisible(is808 || isDrums || isBass);
    dottedDensity.setVisible(is808 || isDrums || isBass);
    eightOhEightLblImg.setVisible(is808);
    
    // Hide these as they are replaced by the generic "style" label.
    bassSelectorLblImg.setVisible(false);
    drumsSelectorLblImg.setVisible(false);
    
    // --- Buttons ---
    btnRolls.setVisible(isDrums);

}

void BoomAudioProcessorEditor::regenerate()
{
    auto engine = (boom::Engine)(int)proc.apvts.getRawParameterValue("engine")->load();
    const int bars = barsFromBox(barsBox);

    if (engine == boom::Engine::Drums)
    {
        if (proc.getDrumPattern().isEmpty())
            proc.setDrumPattern(makeDemoPatternDrums(bars));
        drumGrid.setPattern(proc.getDrumPattern());
    }
    else
    {
        if (proc.getMelodicPattern().isEmpty())
            proc.setMelodicPattern(makeDemoPatternMelodic(bars));
        pianoRoll.setPattern(proc.getMelodicPattern());
    }

    repaint();
}

void BoomAudioProcessorEditor::toggleDrumCell(int row, int tick)
{
    auto pat = proc.getDrumPattern();
    for (int i = 0; i < pat.size(); ++i)
        if (pat[i].row == row && pat[i].startTick == tick)
        {
            pat.remove(i); proc.setDrumPattern(pat); drumGrid.setPattern(pat); repaint(); return;
        }
    BoomAudioProcessor::Note n; n.row = row; n.startTick = tick; n.lengthTicks = 24; n.velocity = 100; n.pitch = 0;
    pat.add(n); proc.setDrumPattern(pat); drumGrid.setPattern(pat); repaint();
}

BoomAudioProcessor::Pattern BoomAudioProcessorEditor::makeDemoPatternDrums(int bars) const
{
    BoomAudioProcessor::Pattern pat;
    const int stepsPerBar = 16; const int ticksPerStep = 24;
    const int totalSteps = stepsPerBar * juce::jmax(1, bars);
    for (int c = 0; c < totalSteps; ++c)
    {
        if (c % stepsPerBar == 0) { pat.add({ 0, 0, c * ticksPerStep, 24, 110 }); }
        if (c % stepsPerBar == 8) { pat.add({ 0, 0, c * ticksPerStep, 24, 105 }); }
        if (c % 4 == 0) { pat.add({ 0, 2, c * ticksPerStep, 12,  80 }); }
        if (c % stepsPerBar == 4) { pat.add({ 0, 1, c * ticksPerStep, 24, 110 }); }
        if (c % stepsPerBar == 12) { pat.add({ 0, 1, c * ticksPerStep, 24, 110 }); }
    }
    return pat;
}

BoomAudioProcessor::Pattern BoomAudioProcessorEditor::makeDemoPatternMelodic(int bars) const
{
    BoomAudioProcessor::Pattern pat;
    const int ticks = 24;
    const int base = 36; // C2
    for (int b = 0; b < juce::jmax(1, bars); ++b)
    {
        pat.add({ base + 0, 0, (b * 16 + 0) * ticks, 8 * ticks, 100 });
        pat.add({ base + 7, 0, (b * 16 + 8) * ticks, 8 * ticks, 100 });
    }
    return pat;
}

juce::File BoomAudioProcessorEditor::writeTempMidiFile() const
{
    auto engine = (boom::Engine)(int)proc.apvts.getRawParameterValue("engine")->load();
    juce::MidiFile mf;
    if (engine == boom::Engine::Drums)
    {
        boom::midi::DrumPattern mp;
        for (const auto& n : proc.getDrumPattern())
            mp.add({ n.row, n.startTick, n.lengthTicks, n.velocity });
        mf = boom::midi::buildMidiFromDrums(mp, 96);
    }
    else
    {
        boom::midi::MelodicPattern mp;
        for (const auto& n : proc.getMelodicPattern())
            mp.add({ n.pitch, n.startTick, n.lengthTicks, n.velocity, 1 });
        mf = boom::midi::buildMidiFromMelodic(mp, 96);
    }
    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("BOOM_Pattern.mid");
    boom::midi::writeMidiToFile(mf, tmp);
    return tmp;
}

void BoomAudioProcessorEditor::startExternalMidiDrag()
{
    const juce::File f = writeTempMidiFile();
    juce::StringArray files; files.add(f.getFullPathName());
    performExternalDragDropOfFiles(files, true);
}

AIToolsWindow::AIToolsWindow(BoomAudioProcessor& p, std::function<void()> onClose)
    : proc(p), onCloseFn(std::move(onClose))
{
    setLookAndFeel(&boomui::LNF());
    setSize(700, 700);



    // ---- Non-interactive labels from your artwork (no mouse) ----
    auto addLbl = [this](juce::ImageComponent& ic, const juce::String& png)
    {
        ic.setImage(loadSkin(png));
        ic.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(ic);
    };
    addLbl(titleLbl, "aiToolsLbl.png");
    addLbl(selectAToolLbl, "selectAToolLbl.png");
    addLbl(rhythmimickLbl, "rhythmimickLbl.png");
    addLbl(rhythmimickDescLbl, "rhythmimickDescriptionLbl.png");
    addLbl(slapsmithLbl, "slapsmithLbl.png");
    addLbl(slapsmithDescLbl, "slapsmithDescriptionLbl.png");
    addLbl(styleBlenderLbl, "styleBlenderLbl.png");
    addLbl(styleBlenderDescLbl, "styleBlenderDescriptionLbl.png");
    addLbl(beatboxLbl, "beatboxLbl.png");
    addLbl(beatboxDescLbl, "beatboxDescriptionLbl.png");

    addLbl(recordUpTo60LblTop, "recordUpTo60SecLbl.png");
    addLbl(recordUpTo60LblBottom, "recordUpTo60SecLbl.png");

    // ---- Toggles (right side) ----
    addAndMakeVisible(toggleRhythm); setToggleImages(toggleRhythm, "toggleBtnOff", "toggleBtnOn");
    addAndMakeVisible(toggleSlap);   setToggleImages(toggleSlap, "toggleBtnOff", "toggleBtnOn");
    addAndMakeVisible(toggleBlend);  setToggleImages(toggleBlend, "toggleBtnOff", "toggleBtnOn");
    addAndMakeVisible(toggleBeat);   setToggleImages(toggleBeat, "toggleBtnOff", "toggleBtnOn");

    // ---- Rhythmimick row (recording block) ----
    addAndMakeVisible(btnRec1);   setButtonImages(btnRec1, "recordBtn");
    addAndMakeVisible(btnStop1);  setButtonImages(btnStop1, "stopBtn");
    addAndMakeVisible(btnGen1);   setButtonImages(btnGen1, "generateBtn");
    addAndMakeVisible(btnSave1);  setButtonImages(btnSave1, "saveMidiBtn");
    addAndMakeVisible(btnDrag1);  setButtonImages(btnDrag1, "dragBtn");

    // ---- Slapsmith row ----
    addAndMakeVisible(btnGen2);   setButtonImages(btnGen2, "generateBtn");
    addAndMakeVisible(btnSave2);  setButtonImages(btnSave2, "saveMidiBtn");
    addAndMakeVisible(btnDrag2);  setButtonImages(btnDrag2, "dragBtn");

    addAndMakeVisible(rhythmSeek);
    rhythmSeek.setLookAndFeel(&boomui::LNF());
    rhythmSeek.setSliderStyle(juce::Slider::LinearBar);
    rhythmSeek.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    rhythmSeek.setRange(0.0, 60.0, 0.01);
    rhythmSeek.setEnabled(false);

    addAndMakeVisible(beatboxSeek);
    beatboxSeek.setLookAndFeel(&boomui::LNF());
    beatboxSeek.setSliderStyle(juce::Slider::LinearBar);
    beatboxSeek.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    beatboxSeek.setRange(0.0, 60.0, 0.01);
    beatboxSeek.setEnabled(false);

    startTimerHz(30); // drive the seek bars ~30 fps

    // ---- Slapsmith row Save/Drag: export only ENABLED rows from miniGrid ----
    btnSave2.onClick = [this]
    {
        juce::File src = miniGrid.exportSelectionToMidiTemp("BOOM_Slapsmith_Selected");
        juce::File defaultDir = src.getParentDirectory();
        juce::File defaultFile = defaultDir.getChildFile("BOOM_Slapsmith_Selected.mid");
        juce::FileChooser fc("Save MIDI...", defaultFile, "*.mid");
        fc.launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [src](const juce::FileChooser& chooser)
            {
                juce::File dest = chooser.getResult();
                if (dest.getFullPathName().isEmpty()) return;
                if (!dest.hasFileExtension(".mid")) dest = dest.withFileExtension(".mid");
                if (dest.existsAsFile()) dest.deleteFile();
                src.copyFileTo(dest);
            });
    };

    btnDrag2.onClick = [this]
    {
        juce::File f = miniGrid.exportSelectionToMidiTemp("BOOM_Slapsmith_Selected");
        if (!f.existsAsFile()) return;
        if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            juce::StringArray files; files.add(f.getFullPathName());
            dnd->performExternalDragDropOfFiles(files, true);
        }
    };

    // ---- Style Blender row (two style sliders in the art; we show density sliders) ----
    addAndMakeVisible(styleA); styleA.setSliderStyle(juce::Slider::LinearHorizontal); styleA.setRange(0, 100, 1); styleA.setValue(85);
    addAndMakeVisible(styleB); styleB.setSliderStyle(juce::Slider::LinearHorizontal); styleB.setRange(0, 100, 1); styleB.setValue(15);
    addAndMakeVisible(btnGen3);   setButtonImages(btnGen3, "generateBtn");
    addAndMakeVisible(btnSave3);  setButtonImages(btnSave3, "saveMidiBtn");
    addAndMakeVisible(btnDrag3);  setButtonImages(btnDrag3, "dragBtn");

    // ---- Beatbox row (recording block) ----
    addAndMakeVisible(btnRec4);   setButtonImages(btnRec4, "recordBtn");
    addAndMakeVisible(btnStop4);  setButtonImages(btnStop4, "stopBtn");
    addAndMakeVisible(btnGen4);   setButtonImages(btnGen4, "generateBtn");
    addAndMakeVisible(btnSave4);  setButtonImages(btnSave4, "saveMidiBtn");
    addAndMakeVisible(btnDrag4);  setButtonImages(btnDrag4, "dragBtn");

    // ---- Home ----
    addAndMakeVisible(btnHome);   setButtonImages(btnHome, "homeBtn");
    btnHome.onClick = [this] { if (onCloseFn) onCloseFn(); };

    // --- Slapsmith mini-grid ---
    addAndMakeVisible(miniGrid);
    miniGrid.setRows(proc.getDrumRows()); // or a subset like first 4 rows
    miniGrid.onCellEdited = [this](int row, int step, bool value)
    {
        // Convert grid step -> ticks (grid uses 24 ticks/step)
        const int startTick = step * 24;

        auto pat = proc.getDrumPattern();
        int found = -1;
        for (int i = 0; i < pat.size(); ++i)
            if (pat[i].row == row && pat[i].startTick == startTick)
            {
                found = i; break;
            }

        if (value) // cell turned ON
        {
            if (found < 0) pat.add({ 0, row, startTick, 24, 100 });
        }
        else        // cell turned OFF
        {
            if (found >= 0) pat.remove(found);
        }

        proc.setDrumPattern(pat);
    };

    // --- Style blender combo boxes ---
    addAndMakeVisible(styleABox); styleABox.addItemList(boom::styleChoices(), 1); styleABox.setSelectedId(1);
    addAndMakeVisible(styleBBox); styleBBox.addItemList(boom::styleChoices(), 1); styleBBox.setSelectedId(2);

    // Wire Generate buttons to correct behavior
    btnGen1.onClick = [this]    // Rhythmimick -> analyze captured audio
    {
        proc.aiStopCapture();                         // make sure we’re not still recording
        proc.aiAnalyzeCapturedToDrums(/*bars*/4, /*bpm*/120);
    };
    btnGen2.onClick = [this]    // Slapsmith -> expand current mini pattern
    {
        proc.aiSlapsmithExpand(/*bars*/4);
    };
    btnGen3.onClick = [this]    // Style Blender
    {
        proc.aiStyleBlendDrums(styleABox.getText(), styleBBox.getText(), /*bars*/4);
    };
    btnGen4.onClick = [this]    // Beatbox -> analyze captured mic audio
    {
        proc.aiStopCapture();
        proc.aiAnalyzeCapturedToDrums(/*bars*/4, /*bpm*/120);
    };

    // Record/Stop behavior per row
    btnRec1.onClick = [this] { proc.aiStartCapture(BoomAudioProcessor::CaptureSource::Loopback); };
    btnStop1.onClick = [this] { proc.aiStopCapture(); };
    btnRec4.onClick = [this] { proc.aiStartCapture(BoomAudioProcessor::CaptureSource::Microphone); };
    btnStop4.onClick = [this] { proc.aiStopCapture(); };




    // ---- Shared actions (Save/Drag) use real MIDI from processor ----
    auto hookupRow = [this](juce::ImageButton& save, juce::ImageButton& drag, const juce::String& baseFile)
    {
        save.onClick = [this, baseFile]
        {
            juce::File src = buildTempMidi(baseFile);
            juce::File defaultDir = src.getParentDirectory();
            juce::File defaultFile = defaultDir.getChildFile(baseFile + ".mid");

            juce::FileChooser fc("Save MIDI...", defaultFile, "*.mid");
            fc.launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [src](const juce::FileChooser& chooser)
                {
                    juce::File dest = chooser.getResult();
                    if (dest.getFullPathName().isEmpty()) return;
                    if (!dest.hasFileExtension(".mid")) dest = dest.withFileExtension(".mid");
                    if (dest.existsAsFile()) dest.deleteFile();
                    src.copyFileTo(dest);
                });
        };
        drag.onClick = [this, baseFile]
        {
            juce::File f = buildTempMidi(baseFile);
            performFileDrag(f);
        };
    };


    hookupRow(btnSave1, btnDrag1, "BOOM_Rhythmimick");
    hookupRow(btnSave2, btnDrag2, "BOOM_Slapsmith");
    hookupRow(btnSave3, btnDrag3, "BOOM_StyleBlender");
    hookupRow(btnSave4, btnDrag4, "BOOM_Beatbox");

    // You can wire Generate/Record/Stop callbacks to your engines here later.
    // For now they're present and clickable (no-op), zero TextButtons.

}
void AIToolsWindow::timerCallback()
{
    // These getters must exist on your processor; if not, add them:
    //   int    getCaptureLengthSamples() const { return captureLengthSamples; }
    //   double getCaptureSampleRate()   const { return lastSampleRate; }
    const double sr = proc.getCaptureSampleRate();
    const int    lenS = proc.getCaptureLengthSamples();
    const double sec = (sr > 0.0) ? (double)lenS / sr : 0.0;

    rhythmSeek.setValue(juce::jlimit(0.0, 60.0, sec), juce::dontSendNotification);
    beatboxSeek.setValue(juce::jlimit(0.0, 60.0, sec), juce::dontSendNotification);
}

AIToolsWindow::~AIToolsWindow()
{
    stopTimer();
    rhythmSeek.setLookAndFeel(nullptr);
    beatboxSeek.setLookAndFeel(nullptr);
}

void AIToolsWindow::paint(juce::Graphics& g)
{
    g.fillAll(boomtheme::MainBackground());
    // If you want a full static background, uncomment:
    // g.drawImageWithin(loadSkin("aiToolsWindowMockUp.png"), 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::fillDestination);
}

void AIToolsWindow::resized()
{
    // Coordinates keyed to your mockup. Everything scales from 700x700.
    const float W = 700.f, H = 700.f;
    auto r = getLocalBounds();
    auto sx = r.getWidth() / W, sy = r.getHeight() / H;
    auto S = [sx, sy](int x, int y, int w, int h)
    {
        return juce::Rectangle<int>(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
            juce::roundToInt(w * sx), juce::roundToInt(h * sy));
    };

    // Title + "Select a tool"
    titleLbl.setBounds(S(160, 24, 400, 88));
    selectAToolLbl.setBounds(S(1300, 80, 80, 102));

    // ---- Rhythmimick row (top block)
    rhythmimickLbl.setBounds(S(420, 190, 200, 55));
    rhythmimickDescLbl.setBounds(S(420, 245, 225, 33));
    recordUpTo60LblTop.setBounds(S(90, 265, 280, 22));

    btnRec1.setBounds(S(90, 230, 50, 50));
    btnStop1.setBounds(S(160, 230, 50, 50));
    btnGen1.setBounds(S(80, 320, 150, 46));
    btnSave1.setBounds(S(240, 320, 150, 46));
    btnDrag1.setBounds(S(400, 320, 170, 46));
    toggleRhythm.setBounds(S(1310, 200, 190, 70)); // big toggle on the right

    // ---- Slapsmith row
    slapsmithLbl.setBounds(S(420, 390, 200, 54));
    slapsmithDescLbl.setBounds(S(420, 445, 225, 44));
    // Slapsmith mini-grid – put below its row labels
    miniGrid.setBounds(S(600, 480, 880, 200));  // adjust to your art

    // Style blender A/B boxes near the style text
    styleABox.setBounds(S(84, 610, 280, 28));
    styleBBox.setBounds(S(84, 670, 280, 28));
    btnGen2.setBounds(S(80, 480, 150, 46));
    btnSave2.setBounds(S(240, 480, 150, 46));
    btnDrag2.setBounds(S(400, 480, 170, 46));
    toggleSlap.setBounds(S(1310, 400, 190, 70));

    // ---- Style Blender row
    styleBlenderLbl.setBounds(S(420, 585, 200, 60));
    styleBlenderDescLbl.setBounds(S(420, 640, 225, 37));
    styleA.setBounds(S(84, 645, 157, 68));
    styleB.setBounds(S(84, 705, 157, 68));
    btnGen3.setBounds(S(80, 740, 150, 46));
    btnSave3.setBounds(S(240, 740, 150, 46));
    btnDrag3.setBounds(S(400, 740, 170, 46));
    toggleBlend.setBounds(S(1310, 600, 190, 70));

    // ---- Beatbox row (bottom)
    beatboxLbl.setBounds(S(420, 820, 200, 60));
    beatboxDescLbl.setBounds(S(420, 875, 335, 100));
    recordUpTo60LblBottom.setBounds(S(90, 905, 280, 30));
    btnRec4.setBounds(S(90, 230, 50, 50));
    btnStop4.setBounds(S(160, 230, 50, 50));
    btnGen4.setBounds(S(80, 320, 150, 46));
    btnSave4.setBounds(S(240, 320, 150, 46));
    btnDrag4.setBounds(S(400, 320, 170, 46));
    toggleBeat.setBounds(S(1310, 900, 190, 70));

    // Home button bottom-right
    btnHome.setBounds(S(1330, 1180, 150, 150));
}

juce::File AIToolsWindow::buildTempMidi(const juce::String& base) const
{
    auto engine = (boom::Engine)(int)proc.apvts.getRawParameterValue("engine")->load();
    juce::MidiFile mf;
    if (engine == boom::Engine::Drums)
    {
        boom::midi::DrumPattern mp;
        for (const auto& n : proc.getDrumPattern())
            mp.add({ n.row, n.startTick, n.lengthTicks, n.velocity });
        mf = boom::midi::buildMidiFromDrums(mp, 96);
    }
    else
    {
        boom::midi::MelodicPattern mp;
        for (const auto& n : proc.getMelodicPattern())
            mp.add({ n.pitch, n.startTick, n.lengthTicks, n.velocity, 1 });
        mf = boom::midi::buildMidiFromMelodic(mp, 96);
    }
    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(base + ".mid");
    boom::midi::writeMidiToFile(mf, tmp);
    return tmp;
}

void AIToolsWindow::performFileDrag(const juce::File& f)
{
    if (!f.existsAsFile()) return;
    if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        juce::StringArray files; files.add(f.getFullPathName());
        dnd->performExternalDragDropOfFiles(files, true);
    }
}

// ================== Modals: ALL ImageButtons ==================
// --- FlippitWindow ---
FlippitWindow::FlippitWindow(BoomAudioProcessor& p,
    std::function<void()> onClose,
    std::function<void(int density)> onFlip,
    boom::Engine engine)
    : proc(p), onCloseFn(std::move(onClose)), onFlipFn(std::move(onFlip))
{
    setLookAndFeel(&boomui::LNF());
    setSize(700, 450);

    const bool isDrums = (engine == boom::Engine::Drums);

    {
        const juce::String lblFile = isDrums ? "flippitDrumsLbl.png" : "flippitLbl.png";
        titleLbl.setImage(boomui::loadSkin(lblFile));
        titleLbl.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(titleLbl);
    }
    // Engine-specific button bases
    const juce::String flipArtBase = isDrums ? "flippitBtnDrums" : "flippitBtn808Bass";
    const juce::String saveArtBase = isDrums ? "saveMidiFlippitDrums" : "saveMidiFlippit808Bass";
    const juce::String dragArtBase = isDrums ? "dragBtnFlippitDrums" : "dragBtnFlippit808Bass";

    // Controls
    addAndMakeVisible(variation);
    variation.setRange(0, 100, 1);
    variation.setValue(35);
    variation.setSliderStyle(juce::Slider::LinearHorizontal);

    addAndMakeVisible(btnFlip);     setButtonImages(btnFlip, flipArtBase);
    addAndMakeVisible(btnSaveMidi); setButtonImages(btnSaveMidi, saveArtBase);
    addAndMakeVisible(btnDragMidi); setButtonImages(btnDragMidi, dragArtBase);
    addAndMakeVisible(btnHome);     setButtonImages(btnHome, "homeBtn");

    btnFlip.onClick = [this]
    {
        if (onFlipFn) onFlipFn((int)juce::jlimit(0.0, 100.0, variation.getValue()));
    };

    btnSaveMidi.onClick = [this]
    {
        juce::File src = buildTempMidi(); // the temp MIDI we generated

        juce::File defaultDir = src.getParentDirectory();
        juce::File defaultFile = defaultDir.getChildFile("BOOM_Flippit.mid");

        juce::FileChooser fc("Save MIDI...", defaultFile, "*.mid");
        fc.launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [src](const juce::FileChooser& chooser)
            {
                juce::File dest = chooser.getResult();
                if (dest.getFullPathName().isEmpty())
                    return; // user cancelled

                if (!dest.hasFileExtension(".mid"))
                    dest = dest.withFileExtension(".mid");

                // overwrite if exists
                if (dest.existsAsFile())
                    dest.deleteFile();

                src.copyFileTo(dest);
            });
    };


    btnDragMidi.onClick = [this]
    {
        juce::File f = buildTempMidi();
        performFileDrag(f);
    };

    btnHome.onClick = [this] { if (onCloseFn) onCloseFn(); };
}

void FlippitWindow::paint(juce::Graphics& g)
{
    g.fillAll(boomtheme::MainBackground());
}

void FlippitWindow::resized()
{
    auto r = getLocalBounds();

    // 700x450 reference
    const float W = 700.f, H = 450.f;
    auto sx = r.getWidth() / W, sy = r.getHeight() / H;
    auto S = [sx, sy](int x, int y, int w, int h)
    {
        return juce::Rectangle<int>(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
            juce::roundToInt(w * sx), juce::roundToInt(h * sy));
    };

    // Center the title at the top using the image's natural size scaled by sx/sy.
    {
        auto img = titleLbl.getImage();
        const int iw = juce::roundToInt(img.getWidth() * sx);
        const int ih = juce::roundToInt(img.getHeight() * sy);
        const int x = (r.getWidth() - iw) / 2;
        const int y = juce::roundToInt(24 * sy);
        titleLbl.setBounds(x, y, iw, ih);
    }

    // Positions tuned to the provided flippit mockups
    btnFlip.setBounds(S(270, 150, 160, 72));
    variation.setBounds(S(40, 250, 620, 24));
    btnSaveMidi.setBounds(S(40, 350, 120, 40));
    btnDragMidi.setBounds(S(220, 340, 260, 50));
    btnHome.setBounds(S(600, 350, 60, 60));
}

// --- Flippit helpers ---
juce::File FlippitWindow::buildTempMidi() const
{
    auto engine = (boom::Engine)(int)proc.apvts.getRawParameterValue("engine")->load();
    juce::MidiFile mf;

    if (engine == boom::Engine::Drums)
    {
        boom::midi::DrumPattern mp;
        for (const auto& n : proc.getDrumPattern())
            mp.add({ n.row, n.startTick, n.lengthTicks, n.velocity });
        mf = boom::midi::buildMidiFromDrums(mp, 96);
    }
    else
    {
        boom::midi::MelodicPattern mp;
        for (const auto& n : proc.getMelodicPattern())
            mp.add({ n.pitch, n.startTick, n.lengthTicks, n.velocity, 1 });
        mf = boom::midi::buildMidiFromMelodic(mp, 96);
    }

    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("BOOM_Flippit.mid");
    boom::midi::writeMidiToFile(mf, tmp);
    return tmp;
}

void FlippitWindow::performFileDrag(const juce::File& f)
{
    if (!f.existsAsFile()) return;
    if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        juce::StringArray files; files.add(f.getFullPathName());
        dnd->performExternalDragDropOfFiles(files, true);
    }
}

// --- BumppitWindow ---
BumppitWindow::BumppitWindow(BoomAudioProcessor& p,
    std::function<void()> onClose,
    std::function<void()> onBump,
    boom::Engine engine)
    : proc(p), onCloseFn(std::move(onClose)), onBumpFn(std::move(onBump))
{
    setLookAndFeel(&boomui::LNF());
    setSize(700, 462);

    // Background per engine
    const bool isDrums = (engine == boom::Engine::Drums);
    // Top label depending on engine
    {
        const juce::String lblFile = isDrums ? "bumppitDrumsLbl.png" : "bumppitLbl.png";
        titleLbl.setImage(boomui::loadSkin(lblFile));
        titleLbl.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(titleLbl);
    }

    // Engine-specific Bumppit button art
    const juce::String bumpArtBase = isDrums ? "bumppitBtnDrums" : "bumppitBtn808Bass";

    addAndMakeVisible(btnBump);  setButtonImages(btnBump, bumpArtBase);
    addAndMakeVisible(btnHome);  setButtonImages(btnHome, "homeBtn");

    btnBump.onClick = [this] { if (onBumpFn) onBumpFn(); };
    btnHome.onClick = [this] { if (onCloseFn) onCloseFn(); };

    // Melodic options only for 808/Bass layout
    showMelodicOptions = !isDrums;
    if (showMelodicOptions)
    {
        addAndMakeVisible(keyBox);    keyBox.addItemList(boom::keyChoices(), 1);     keyBox.setSelectedId(1);
        addAndMakeVisible(scaleBox);  scaleBox.addItemList(boom::scaleChoices(), 1); scaleBox.setSelectedId(1);
        addAndMakeVisible(octaveBox); octaveBox.addItemList(juce::StringArray("-2", "-1", "0", "+1", "+2"), 1); octaveBox.setSelectedId(3);
        addAndMakeVisible(barsBox);   barsBox.addItemList(juce::StringArray("1", "2", "4", "8"), 1);          barsBox.setSelectedId(3);
    }
}

void BumppitWindow::paint(juce::Graphics& g)
{
    g.fillAll(boomtheme::MainBackground());
}

void BumppitWindow::resized()
{
    auto r = getLocalBounds();

    // scale helper for 700x462
    const float W = 700.f, H = 462.f;
    auto sx = r.getWidth() / W, sy = r.getHeight() / H;
    auto S = [sx, sy](int x, int y, int w, int h)
    {
        return juce::Rectangle<int>(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
            juce::roundToInt(w * sx), juce::roundToInt(h * sy));
    };

    // Center the title at the top using the image's natural size scaled by sx/sy.
    {
        auto img = titleLbl.getImage();
        const int iw = juce::roundToInt(img.getWidth() * sx);
        const int ih = juce::roundToInt(img.getHeight() * sy);
        const int x = (r.getWidth() - iw) / 2;
        const int y = juce::roundToInt(24 * sy);
        titleLbl.setBounds(x, y, iw, ih);
    }

    // layout per mockups:
    if (showMelodicOptions)
    {
        // 808/Bass mockup (first image): four combo boxes centered column
        keyBox.setBounds(S(215, 130, 270, 46));
        scaleBox.setBounds(S(215, 180, 270, 46));
        octaveBox.setBounds(S(215, 230, 270, 46));
        barsBox.setBounds(S(215, 280, 270, 46));

        // place the Bumppit button beneath those controls (centered)
        btnBump.setBounds(S(175, 340, 350, 74));
    }
    else
    {
        // Drums mockup (second image): one big Bumppit button in the middle
        btnBump.setBounds(S(130, 171, 440, 120));
    }

    // Home button bottom-right (as in mockups)
    btnHome.setBounds(S(620, 382, 60, 60));
}

RollsWindow::RollsWindow(BoomAudioProcessor& p, std::function<void()> onClose, std::function<void(juce::String, int, int)> onGen)
    : proc(p), onCloseFn(std::move(onClose)), onGenerateFn(std::move(onGen))
{
    setLookAndFeel(&boomui::LNF());
    setSize(700, 447);

    // STYLE box (same list as main)
    styleBox.addItemList(boom::styleChoices(), 1);
    styleBox.setSelectedId(1, juce::dontSendNotification);

    // BARS box (Rolls-specific: 1,2,4,8)
    barsBox.clear();
    barsBox.addItem("1", 1);
    barsBox.addItem("2", 2);
    barsBox.addItem("4", 3);
    barsBox.addItem("8", 4);
    barsBox.setSelectedId(3, juce::dontSendNotification); // default to "4"


    addAndMakeVisible(rollsTitleImg);
    rollsTitleImg.setInterceptsMouseClicks(false, false);
    rollsTitleImg.setImage(loadSkin("rollGerneratorLbl.png")); // exact filename from your asset list
    rollsTitleImg.setImagePlacement(juce::RectanglePlacement::centred); // keep aspect when we size the box
    addAndMakeVisible(variation); variation.setRange(0, 100, 1); variation.setValue(35); variation.setSliderStyle(juce::Slider::LinearHorizontal);


    addAndMakeVisible(btnGenerate); setButtonImages(btnGenerate, "generateBtn");
    addAndMakeVisible(btnClose);    setButtonImages(btnClose, "homeBtn");
    addAndMakeVisible(btnSaveMidi); setButtonImages(btnSaveMidi, "saveMidiFlippitDrums");
    addAndMakeVisible(btnDragMidi); setButtonImages(btnDragMidi, "dragBtnFlippitDrums");

    btnGenerate.onClick = [this]
    {
        const juce::String style = styleBox.getText();

        int bars = 4;
        switch (barsBox.getSelectedId())
        {
        case 1: bars = 1; break;
        case 2: bars = 2; break;
        case 3: bars = 4; break;
        case 4: bars = 8; break;
        default: bars = 4; break;
        }

        proc.generateRolls(style, bars);
    };

    btnClose.onClick = [this] { if (onCloseFn) onCloseFn(); };

    btnSaveMidi.onClick = [this]
    {
        juce::File src = buildTempMidi();

        juce::File defaultDir = src.getParentDirectory();
        juce::File defaultFile = defaultDir.getChildFile("BOOM_Roll.mid");

        juce::FileChooser fc("Save MIDI...", defaultFile, "*.mid");
        fc.launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [src](const juce::FileChooser& chooser)
            {
                juce::File dest = chooser.getResult();
                if (dest.getFullPathName().isEmpty())
                    return;

                if (!dest.hasFileExtension(".mid"))
                    dest = dest.withFileExtension(".mid");

                if (dest.existsAsFile())
                    dest.deleteFile();

                src.copyFileTo(dest);
            });
    };

    btnDragMidi.onClick = [this]
    {
        juce::File f = buildTempMidi();
        performFileDrag(f);
    };
}
void RollsWindow::paint(juce::Graphics& g)
{
    g.fillAll(boomtheme::MainBackground());
}

void RollsWindow::resized()
{
    auto r = getLocalBounds();
    auto bounds = getLocalBounds();

    // Pick a target width for the label (scaled to your layout). Adjust to taste.
    const int targetW = juce::jmin(bounds.getWidth() - 40, 420); // clamp <= 420px wide with 20px margins
    const auto img = rollsTitleImg.getImage();

    int targetH = 64; // default if image missing
    if (img.isValid())
        targetH = (int)std::round((double)img.getHeight() * (double)targetW / (double)img.getWidth());

    const int x = (bounds.getWidth() - targetW) / 2;
    const int y = 20; // top padding

    rollsTitleImg.setBounds(x, y, targetW, targetH);

    // 700x447 reference
    const float W = 700.f, H = 447.f;
    auto sx = r.getWidth() / W, sy = r.getHeight() / H;
    auto S = [sx, sy](int x, int y, int w, int h)
    {
        return juce::Rectangle<int>(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
            juce::roundToInt(w * sx), juce::roundToInt(h * sy));
    };
    rollsTitleImg.setBounds(S(350, 15, 258, 131));
    styleBox.setBounds(S(240, 175, 100, 20));
    barsBox.setBounds(S(350, 175, 50, 20));
    btnGenerate.setBounds(S(225, 225, 190, 60));

    btnSaveMidi.setBounds(S(40, 350, 120, 40));
    btnDragMidi.setBounds(S(190, 345, 280, 50));
    btnClose.setBounds(S(600, 350, 60, 60));
}

juce::File RollsWindow::buildTempMidi() const
{
    juce::MidiFile mf;
    boom::midi::DrumPattern mp;
    for (const auto& n : proc.getDrumPattern())
        mp.add({ n.row, n.startTick, n.lengthTicks, n.velocity });
    mf = boom::midi::buildMidiFromDrums(mp, 96);

    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("BOOM_Roll.mid");
    boom::midi::writeMidiToFile(mf, tmp);
    return tmp;
}

void RollsWindow::performFileDrag(const juce::File& f)
{
    if (!f.existsAsFile()) return;
    if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        juce::StringArray files; files.add(f.getFullPathName());
        dnd->performExternalDragDropOfFiles(files, true);
    }
}