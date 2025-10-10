#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Theme.h"

class DrumGridComponent : public juce::Component
{
public:
    explicit DrumGridComponent(BoomAudioProcessor& p, int barsToShow = 4, int stepsPerBar_ = 16)
        : proc(p), bars(barsToShow), stepsPerBar(stepsPerBar_)
    {
        setWantsKeyboardFocus(true);
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        setInterceptsMouseClicks(true, true);

        setRows(proc.getDrumRows());  // default names from processor
        clearGrid();
    }

    // Set the visible row names (e.g., Kick, Snare, Hat, Tom…)
    void setRows(const juce::StringArray& names)
    {
        rowNames = names;
        const int r = juce::jmax(1, rowNames.size());
        cells.resize((size_t)r);
        rowEnabled.resize((size_t)r, true);
        for (auto& row : cells) row.assign(totalSteps(), false);
        repaint();
    }

    // Push an existing drum pattern into the grid (marks cells true where notes exist).
    // Assumes: row field of Note is the drum row index; startTick quantized at 16th (24 ticks).
    void setPattern(const BoomAudioProcessor::Pattern& pat)
    {
        clearGrid();
        for (const auto& n : pat)
        {
            if (n.row < 0 || n.row >= (int)cells.size()) continue;
            const int step = (n.startTick / ticksPerStep) % totalSteps();
            if (step >= 0 && step < totalSteps())
                cells[(size_t)n.row][(size_t)step] = true;
        }
        repaint();
    }

    // Read out the grid into a Pattern (all rows), for internal use.
    BoomAudioProcessor::Pattern getPatternAllRows() const
    {
        BoomAudioProcessor::Pattern p;
        for (int r = 0; r < (int)cells.size(); ++r)
            for (int s = 0; s < totalSteps(); ++s)
                if (cells[(size_t)r][(size_t)s])
                    p.add({ 0, r, s * ticksPerStep, ticksPerStep, 100 });
        return p;
    }

    // Read out only **enabled** rows (for filtered export).
    BoomAudioProcessor::Pattern getPatternEnabledRows() const
    {
        BoomAudioProcessor::Pattern p;
        for (int r = 0; r < (int)cells.size(); ++r)
        {
            if (!rowEnabled[(size_t)r]) continue;
            for (int s = 0; s < totalSteps(); ++s)
                if (cells[(size_t)r][(size_t)s])
                    p.add({ 0, r, s * ticksPerStep, ticksPerStep, 100 });
        }
        return p;
    }

    // Quick export of the **enabled rows only** to a temp MIDI file
    juce::File exportSelectionToMidiTemp(const juce::String& baseFileName) const
    {
        auto pat = getPatternEnabledRows();
        // Build a BOOM drum MIDI (96 tpq) from our grid
        juce::MidiFile mf;
        {
            boom::midi::DrumPattern mp;
            for (const auto& n : pat)
                mp.add({ n.row, n.startTick, n.lengthTicks, n.velocity });
            mf = boom::midi::buildMidiFromDrums(mp, 96);
        }
        auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(baseFileName + ".mid");
        boom::midi::writeMidiToFile(mf, tmp);
        return tmp;
    }

    // External hook (optional): editor can observe toggles
    std::function<void(int row, int tick)> onToggle;
    std::function<void(int row, int step, bool value)> onCellEdited;

    // ================= Component =================
    void paint(juce::Graphics& g) override
    {
        using namespace boomtheme;
        g.fillAll(GridBackground());

        auto r = getLocalBounds().toFloat();
        const float labelWf = labelWidth();
        const float gridX = r.getX() + labelWf;
        const float gridW = r.getWidth() - labelWf;

        const int R = juce::jmax(1, (int)cells.size());
        const int C = totalSteps();

        const float cellH = r.getHeight() / (float)R;
        const float cellW = gridW / (float)C;

        // Row label area
        for (int row = 0; row < R; ++row)
        {
            auto rowY = r.getY() + row * cellH;

            // background strip for label
            g.setColour(boomtheme::GridBackground());
            g.fillRect(juce::Rectangle<float>(r.getX(), rowY, labelWf, cellH));
            g.setColour(juce::Colours::black);
            g.drawRect(juce::Rectangle<float>(r.getX(), rowY, labelWf, cellH), 1.2f);

            // label text/image: we’ll render text (you can change to images if you have per-row art)
            const auto name = rowNames[row];
            g.setColour(rowEnabled[(size_t)row] ? juce::Colours::white : juce::Colours::grey);
            g.setFont(juce::Font(14.0f, juce::Font::bold));
            g.drawFittedText(name, juce::Rectangle<int>((int)r.getX() + 6, (int)rowY, (int)labelWf - 12, (int)cellH), juce::Justification::centredLeft, 1);
        }

        // Grid background
        g.setColour(GridBackground());
        g.fillRect(juce::Rectangle<float>(gridX, r.getY(), gridW, r.getHeight()));

        // Grid lines
        g.setColour(GridLine());
        for (int c = 0; c <= C; ++c)
        {
            const float x = gridX + c * cellW;
            const float thickness = (c % stepsPerBar == 0 ? 1.6f : (c % 4 == 0 ? 1.1f : 0.6f));
            g.drawLine(x, r.getY(), x, r.getBottom(), thickness);
        }
        for (int row = 0; row <= R; ++row)
        {
            const float y = r.getY() + row * cellH;
            g.drawLine(gridX, y, gridX + gridW, y, 0.6f);
        }

        // Cells
        for (int row = 0; row < R; ++row)
        {
            const bool enabled = rowEnabled[(size_t)row];
            for (int c = 0; c < C; ++c)
            {
                const auto x = gridX + c * cellW;
                const auto y = r.getY() + row * cellH;
                auto cellR = juce::Rectangle<float>(x + 2.f, y + 2.f, cellW - 4.f, cellH - 4.f);

                if (cells[(size_t)row][(size_t)c])
                {
                    g.setColour(enabled ? NoteFill() : NoteFill().darker(0.6f));
                    g.fillRoundedRectangle(cellR, 3.5f);
                    g.setColour(juce::Colours::black);
                    g.drawRoundedRectangle(cellR, 3.5f, 1.2f);
                }
                else
                {
                    if (!enabled)
                    {
                        g.setColour(PanelStroke().withAlpha(0.15f));
                        g.fillRoundedRectangle(cellR, 3.5f);
                    }
                }
            }
        }
    }

    void resized() override {}

    // ================= Interaction =================

    void mouseDown(const juce::MouseEvent& e) override
    {
        const Hit h = hitTest(e.position);
        if (!h.valid) return;

        if (h.onLabel)
        {
            // Toggle row enabled/disabled
            const bool now = !(rowEnabled[(size_t)h.row]);
            rowEnabled[(size_t)h.row] = now;
            repaint();
            return;
        }

        // Start paint sweep on this row
        dragging = true;
        dragRow = h.row;
        dragValue = !cells[(size_t)h.row][(size_t)h.step]; // invert current cell and paint that value
        setCell(h.row, h.step, dragValue);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!dragging) return;

        const Hit h = hitTest(e.position);
        if (!h.valid) return;

        // Constrain sweep to the same row where mouseDown happened
        if (h.row != dragRow) return;

        setCell(h.row, h.step, dragValue);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        dragging = false;
        dragRow = -1;
    }

private:
    BoomAudioProcessor& proc;

    juce::StringArray rowNames;
    std::vector<std::vector<bool>> cells;   // [row][step]
    std::vector<bool> rowEnabled;

    const int stepsPerBar = 16;
    const int ticksPerStep = 24;
    const int bars;
    bool dragging = false;
    int dragRow = -1;
    bool dragValue = false;

    struct Hit
    {
        bool valid = false;
        bool onLabel = false;
        int row = -1;
        int step = -1;
    };

    int totalSteps() const { return bars * stepsPerBar; }
    float labelWidth() const { return juce::jmax(120.0f, getWidth() * 0.12f); }

    void clearGrid()
    {
        const int R = juce::jmax(1, rowNames.size());
        cells.resize((size_t)R);
        for (auto& row : cells) row.assign(totalSteps(), false);
    }

    void setCell(int row, int step, bool v)
    {
        if (row < 0 || row >= (int)cells.size()) return;
        if (step < 0 || step >= totalSteps())    return;
        if (!rowEnabled[(size_t)row])           return; // ignore edits when row disabled

        if (cells[(size_t)row][(size_t)step] == v) return;
        cells[(size_t)row][(size_t)step] = v;

        if (onCellEdited) onCellEdited(row, step, v);
        if (onToggle) onToggle(row, step * ticksPerStep);
        repaint();
    }

    Hit hitTest(juce::Point<float> p) const
    {
        Hit h;
        auto r = getLocalBounds().toFloat();
        if (!r.contains(p)) return h;

        const int R = (int)cells.size();
        if (R <= 0) return h;

        const float labelWf = labelWidth();
        const float gridX = r.getX() + labelWf;
        const float gridW = r.getWidth() - labelWf;

        const float cellH = r.getHeight() / (float)R;
        const float cellW = gridW / (float)totalSteps();

        h.row = juce::jlimit(0, R - 1, (int)((p.y - r.getY()) / cellH));

        if (p.x < gridX)
        {
            h.onLabel = true;
            h.valid = true;
            return h;
        }

        h.onLabel = false;
        int step = (int)((p.x - gridX) / cellW);
        h.step = juce::jlimit(0, totalSteps() - 1, step);
        h.valid = true;
        return h;
    }
};
