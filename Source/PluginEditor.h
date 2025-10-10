#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EngineDefs.h"
#include "DrumGridComponent.h"
#include "PianoRollComponent.h"
#include "MidiUtils.h"
#include "FlipUtils.h"
#include "PatternAdapters.h"

// === shared UI helpers (inline so header users can see/inline them) ===
namespace boomui
{
    inline juce::Image loadSkin(const juce::String& fileName)
    {
        // try ../Resources/<file>, then CWD
        auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
        auto res1 = exeDir.getChildFile("Resources").getChildFile(fileName);
        if (res1.existsAsFile()) return juce::ImageFileFormat::loadFrom(res1);

        auto res2 = juce::File::getCurrentWorkingDirectory().getChildFile(fileName);
        if (res2.existsAsFile()) return juce::ImageFileFormat::loadFrom(res2);

        return {};
    }

    inline void setButtonImages(juce::ImageButton& b, const juce::String& baseNoExt)
    {
        b.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        b.setImages(false, true, true,
            loadSkin(baseNoExt + ".png"), 1.0f, juce::Colour(),
            loadSkin(baseNoExt + "_hover.png"), 1.0f, juce::Colour(),
            loadSkin(baseNoExt + "_down.png"), 1.0f, juce::Colour());
    }

    inline void setToggleImages(juce::ImageButton& b,
        const juce::String& offBaseNoExt,
        const juce::String& onBaseNoExt)
    {
        auto apply = [&b, offBaseNoExt, onBaseNoExt]()
        {
            const auto& base = b.getToggleState() ? onBaseNoExt : offBaseNoExt;
            setButtonImages(b, base);
        };
        b.setClickingTogglesState(true);
        b.onStateChange = apply;
        apply();
    }
} // namespace boomui

namespace boomui
{
    inline void setButtonImagesSelected(juce::ImageButton& b, const juce::String& baseNoExt)
    {
        // Force the _down image in *all* states to keep the selected visual
        b.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        auto imgDown = loadSkin(baseNoExt + "_down.png");
        b.setImages(false, true, true,
            imgDown, 1.0f, juce::Colour(),
            imgDown, 1.0f, juce::Colour(),
            imgDown, 1.0f, juce::Colour());
    }
}
using boomui::setButtonImagesSelected;

class BoomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float /*min*/, float /*max*/,
        const juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        using namespace boomtheme;
        const auto purple = HeaderBackground();  // your project purple
        const int  trackH = juce::jmax(6, height / 10);
        auto track = juce::Rectangle<int>(x, y + (height - trackH) / 2, width, trackH);

        // Track outline (thick black), then fill
        g.setColour(juce::Colours::black);
        g.drawRect(track.toFloat(), 3.0f);
        g.setColour(purple);
        g.fillRect(track.reduced(3));

        // Thumb as rounded rectangle with black outline
        const int thW = juce::jmax(14, height / 4);
        auto thumb = juce::Rectangle<int>(thW, height - (height / 5)).withCentre(juce::Point<int>((int)sliderPos, y + height / 2));
        auto thumbF = thumb.withHeight(juce::jmax(18, trackH + 8)).toFloat();

        g.setColour(purple);
        g.fillRoundedRectangle(thumbF, 4.0f);
        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(thumbF, 4.0f, 3.0f);

        juce::ignoreUnused(style, s);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional,
        float rotaryStartAngle, float rotaryEndAngle, juce::Slider& s) override
    {
        using namespace boomtheme;
        const auto purple = HeaderBackground();
        auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(4);

        // Body
        g.setColour(purple);
        g.fillEllipse(bounds);
        g.setColour(juce::Colours::black);
        g.drawEllipse(bounds, 4.0f);

        // Pointer
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 6.0f;
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        juce::Path p;
        p.addRectangle(-2.0f, -radius, 4.0f, radius * 0.9f);
        g.setColour(juce::Colours::black);
        g.fillPath(p, juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));

        juce::ignoreUnused(s);
    }
};

// global accessor (one instance used by all windows)
namespace boomui { inline BoomLookAndFeel& LNF() { static BoomLookAndFeel i; return i; } }

// bring into current namespace for existing calls
using boomui::loadSkin;
using boomui::setButtonImages;
using boomui::setToggleImages;

class FlippitWindow;
class BumppitWindow;
class RollsWindow;
class AIToolsWindow;

class BoomAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::DragAndDropContainer
{
public:
    explicit BoomAudioProcessorEditor(BoomAudioProcessor&);
    ~BoomAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override; // start file-drag from dragBtn



private:
    // Layout helpers
    static int barsFromBox(const juce::ComboBox& b);

    void setEngine(boom::Engine e);
    void syncVisibility();
    void regenerate();
    void startExternalMidiDrag();

    void toggleDrumCell(int row, int tick);
    BoomAudioProcessor::Pattern makeDemoPatternDrums(int bars) const;
    BoomAudioProcessor::Pattern makeDemoPatternMelodic(int bars) const;
    juce::File writeTempMidiFile() const;

    BoomAudioProcessor& proc;

    // === Background ===
    juce::ImageComponent background; // boommock.png

    // === Engine buttons (ImageButtons) ===
    juce::ImageButton btn808, btnBass, btnDrums;

    // === Left column controls ===
    juce::ComboBox timeSigBox, barsBox;
    juce::Slider humanizeTiming, humanizeVelocity, swing, tripletDensity, dottedDensity;

    // Dice button + switches as ImageButton toggles
    juce::ImageButton diceBtn;
    juce::ImageButton useTriplets, useDotted;
    juce::ImageComponent soundsDopeLbl;


    // Melodic
    juce::ComboBox keyBox, scaleBox, octaveBox, bassStyleBox;
    juce::Slider   rest808;

    // Drums
    juce::ComboBox drumStyleBox;
    juce::Slider   restDrums;

    // === Action buttons (ImageButtons) ===
    juce::ImageButton btnGenerate;  // generateBtn*.png
    juce::ImageButton btnDragMidi;  // dragBtn*.png
    juce::ImageButton btnFlippit;   // flippitBtn*.png
    juce::ImageButton btnBumppit;   // bumppitBtn*.png
    juce::ImageButton btnRolls;     // rollsBtn*.png
    juce::ImageButton btnAITools;   // aiToolsBtn*.png

    // === Label images (EXACT art) ===
    juce::ImageComponent logoImg;
    juce::ImageComponent engineLblImg;
    juce::ImageComponent scaleLblImg, timeSigLblImg, barsLblImg, humanizeLblImg, styleLblImg;
    juce::ImageComponent tripletsLblImg, dottedNotesLblImg, restDensityLblImg;
    juce::ImageComponent keyLblImg, octaveLblImg;
    juce::ImageComponent bassSelectorLblImg, drumsSelectorLblImg;
    juce::ImageComponent eightOhEightLblImg; // "808BassLbl.png"

    // Views
    DrumGridComponent   drumGrid;
    PianoRollComponent  pianoRoll;

    using Attachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<Attachment> timeSigAtt, barsAtt, keyAtt, scaleAtt, octaveAtt, bassStyleAtt, drumStyleAtt;
    std::unique_ptr<SAttachment> humanizeTimingAtt, humanizeVelocityAtt, swingAtt, rest808Att, restDrumsAtt, tripletDensityAtt, dottedDensityAtt;
    std::unique_ptr<BAttachment> useTripletsAtt, useDottedAtt;

    std::unique_ptr<AIToolsWindow> aiTools;
    std::unique_ptr<FlippitWindow> flippit;
    std::unique_ptr<BumppitWindow> bumppit;
    std::unique_ptr<RollsWindow>  rolls;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoomAudioProcessorEditor)
};

// ===== Sub windows  ALL buttons are ImageButtons =====
class FlippitWindow : public juce::Component
{
public:
    // Engine decides layout + art (background + three buttons).
    FlippitWindow(BoomAudioProcessor& p,
        std::function<void()> onClose,
        std::function<void(int density)> onFlip,
        boom::Engine engine);

    void resized() override;

private:
    BoomAudioProcessor& proc;
    std::function<void()> onCloseFn;
    std::function<void(int)> onFlipFn;

    // Background swaps to flippitBassMockUp.png or flippitDrumsMockUp.png
    void paint(juce::Graphics& g) override;

    // Controls common to both layouts
    juce::Slider variation; // "Variation Density"

    // Buttons (engine-specific art)
    juce::ImageComponent titleLbl;
    juce::ImageButton btnFlip;     // flippitBtn808Bass* OR flippitBtnDrums*
    juce::ImageButton btnSaveMidi; // saveMidiFlippit808Bass* OR saveMidiFlippitDrums*
    juce::ImageButton btnDragMidi; // dragBtnFlippit808Bass* OR dragBtnFlippitDrums*
    juce::ImageButton btnHome;     // homeBtn*

    // helpers
    juce::File buildTempMidi() const;
    void performFileDrag(const juce::File& f);
};

class BumppitWindow : public juce::Component
{
public:
    // Engine decides layout + art.
    BumppitWindow(BoomAudioProcessor& p,
        std::function<void()> onClose,
        std::function<void()> onBump,
        boom::Engine engine);


    void resized() override;

private:
    BoomAudioProcessor& proc;
    std::function<void()> onCloseFn, onBumpFn;

    // Background swaps to bumppitBassMockup.png or bumppitDrumsMockup.png
    void paint(juce::Graphics& g) override;

    // Buttons (art depends on engine)
    juce::ImageComponent titleLbl;
    juce::ImageButton btnBump;   // bumppitBtn808Bass* OR bumppitBtnDrums*
    juce::ImageButton btnHome;   // homeBtn*

    // Controls shown ONLY in 808/Bass layout
    juce::ComboBox keyBox, scaleBox, octaveBox, barsBox;
    bool showMelodicOptions = false;
};

class RollsWindow : public juce::Component
{
public:
    RollsWindow(BoomAudioProcessor& p, std::function<void()> onClose, std::function<void(juce::String, int, int)> onGen);
    void resized() override;
private:
    BoomAudioProcessor& proc;
    juce::ImageComponent rollsTitleImg;
    juce::ComboBox styleBox, barsBox;
    juce::Slider variation;
    juce::ImageButton btnGenerate; // generateBtn*.png
    juce::ImageButton btnClose;    // mainWindowBtn*.png
    std::function<void()> onCloseFn;
    std::function<void(juce::String, int, int)> onGenerateFn;
    void paint(juce::Graphics& g) override;
    juce::ImageButton btnSaveMidi, btnDragMidi;
    juce::File buildTempMidi() const;
    void performFileDrag(const juce::File& f);
};

// ================= AIToolsWindow (all ImageButtons, all art from your set) =================
class AIToolsWindow : public juce::Component
{
public:
    AIToolsWindow(BoomAudioProcessor& p, std::function<void()> onClose);
    ~AIToolsWindow() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    DrumGridComponent miniGrid{ proc }; // if your ctor needs a proc, adjust accordingly
    juce::ComboBox styleABox, styleBBox;

private:
    BoomAudioProcessor& proc;
    std::function<void()> onCloseFn;

    // Labels (all ImageComponent)
    juce::ImageComponent titleLbl, selectAToolLbl;
    juce::ImageComponent rhythmimickLbl, rhythmimickDescLbl, recordUpTo60LblTop;
    juce::ImageComponent slapsmithLbl, slapsmithDescLbl;
    juce::ImageComponent styleBlenderLbl, styleBlenderDescLbl;
    juce::ImageComponent beatboxLbl, beatboxDescLbl, recordUpTo60LblBottom;

    // Toggles
    juce::ImageButton toggleRhythm, toggleSlap, toggleBlend, toggleBeat;

    // Rhythmimick buttons
    juce::ImageButton btnRec1, btnStop1, btnGen1, btnSave1, btnDrag1;

    // Slapsmith buttons
    juce::ImageButton btnGen2, btnSave2, btnDrag2;

    // Style Blender controls
    juce::Slider      styleA, styleB;
    juce::ImageButton btnGen3, btnSave3, btnDrag3;

    // Beatbox buttons
    juce::ImageButton btnRec4, btnStop4, btnGen4, btnSave4, btnDrag4;

    // Home
    juce::ImageButton btnHome;

    // helpers (same pattern export used in other windows)
    juce::File buildTempMidi(const juce::String& base) const;
    void performFileDrag(const juce::File& f);
};
