#include "PianoRollComponent.h"

PianoRollComponent::PianoRollComponent(BoomAudioProcessor& p)
    : processor(p)
{
}

void PianoRollComponent::setPattern(const BoomAudioProcessor::Pattern& pat)
{
    pattern = pat;
    repaint();
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    using namespace boomtheme;
    g.fillAll(GridBackground());

    auto full = getLocalBounds();

    const int keyWidth = 60;      // left piano key band
    const int headerH = 22;      // top header bar
    const int totalBars = 4;      // matches BANG default (change to 8 for longer patterns)

    auto header = full.removeFromTop(headerH);
    auto keyArea = full.removeFromLeft(keyWidth);
    auto r = full;

    const int cols = totalBars * 16;  // 16 steps per bar
    const int rowsCount = 36;         // covers 3 octaves visually like BANG
    const float cellW = r.getWidth() / (float)cols;
    const float cellH = r.getHeight() / (float)rowsCount;

    // header
    g.setColour(HeaderBackground());
    g.fillRect(header);

    // bar markers
    for (int bar = 0; bar <= totalBars; ++bar)
    {
        const int x = r.getX() + (int)std::round(bar * 16 * cellW);
        if (bar < totalBars)
        {
            g.setColour(LightAccent());
            g.drawText(juce::String(bar + 1) + ".1", x + 4, header.getY() + 2,
                48, header.getHeight() - 4, juce::Justification::left);
        }
        g.setColour(LightAccent().withAlpha(0.35f));
        g.drawLine((float)x, (float)header.getY(), (float)x,
            (float)(header.getBottom() + r.getHeight()), 2.0f);
    }

    // grid lines
    g.setColour(GridLine());
    for (int c = 0; c <= cols; ++c)
        g.drawVerticalLine(r.getX() + (int)std::round(c * cellW),
            (float)r.getY(), (float)r.getBottom());
    for (int rr = 0; rr <= rowsCount; ++rr)
        g.drawHorizontalLine(r.getY() + (int)std::round(rr * cellH),
            (float)r.getX(), (float)r.getRight());

    // draw piano key area on the left
    g.setColour(HeaderBackground().darker(0.2f));
    g.fillRect(keyArea);

    // draw white & black key pattern like BANG
    const int baseMidi = 36; // C2
    for (int i = 0; i < rowsCount; ++i)
    {
        int midi = baseMidi + (rowsCount - 1 - i);
        int noteInOctave = midi % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 ||
            noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10);

        g.setColour(isBlackKey ? GridBackground().darker(0.4f)
            : GridBackground().brighter(0.15f));
        const float rowY = (float)r.getY() + i * cellH;
        g.fillRect(juce::Rectangle<float>((float)keyArea.getX(), rowY, (float)keyWidth, (float)cellH));
    }

    // outline the keys
    g.setColour(GridLine());
    for (int rr = 0; rr <= rowsCount; ++rr)
        g.drawHorizontalLine(r.getY() + (int)std::round(rr * cellH),
            (float)keyArea.getX(), (float)keyArea.getRight());

    // draw notes
    g.setColour(NoteFill());
    for (const auto& n : pattern)
    {
        const int col = (n.startTick / 24) % cols;
        const int row = juce::jlimit(0, rowsCount - 1,
            rowsCount - 1 - ((n.pitch - baseMidi) % rowsCount));
        const float w = cellW * juce::jmax(1, n.lengthTicks / 24) - 4.f;

        g.fillRoundedRectangle(
            juce::Rectangle<float>(r.getX() + col * cellW + 2.f,
                r.getY() + row * cellH + 2.f,
                w,
                cellH - 4.f),
            4.f);
    }
}
