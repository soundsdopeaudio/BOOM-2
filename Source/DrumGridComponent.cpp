#include "DrumGridComponent.h"
#include "Theme.h"

DrumGridComponent::DrumGridComponent(BoomAudioProcessor& p)
    : processor(p),
    rows(boom::defaultDrumRows())
{
}

void DrumGridComponent::setRows(const juce::StringArray& r)
{
    rows = r;
    repaint();
}

void DrumGridComponent::setPattern(const BoomAudioProcessor::Pattern& pat)
{
    pattern = pat;
    repaint();
}

const BoomAudioProcessor::Pattern& DrumGridComponent::getPattern() const noexcept
{
    return pattern;
}

void DrumGridComponent::paint(juce::Graphics& g)
{
    using namespace boomtheme;
    g.fillAll(GridBackground());

    auto full = getLocalBounds();
    const int headerH = 22;
    auto header = full.removeFromTop(headerH);
    auto r = full;

    const int R = juce::jmax(1, rows.size());
    const int C = 64;
    const float cellW = r.getWidth() / (float)C;
    const float cellH = r.getHeight() / (float)R;

    g.setColour(HeaderBackground());
    g.fillRect(header);

    for (int bar = 0; bar <= 4; ++bar)
    {
        const int x = r.getX() + (int)std::round(bar * 16 * cellW);
        if (bar < 4)
        {
            g.setColour(LightAccent());
            g.drawText(juce::String(bar + 1) + ".1", x + 4, header.getY() + 2,
                48, header.getHeight() - 4, juce::Justification::left);
        }
        g.setColour(LightAccent().withAlpha(0.35f));
        g.drawLine((float)x, (float)header.getY(), (float)x,
            (float)(header.getBottom() + r.getHeight()), 2.0f);
    }

    g.setColour(GridLine());
    for (int c = 0; c <= C; ++c)
    {
        const int x = r.getX() + (int)std::round(c * cellW);
        g.drawVerticalLine(x, (float)r.getY(), (float)r.getBottom());
    }
    for (int rr = 0; rr <= R; ++rr)
    {
        const int y = r.getY() + (int)std::round(rr * cellH);
        g.drawHorizontalLine(y, (float)r.getX(), (float)r.getRight());
    }

    g.setColour(LightAccent());
    for (int rr = 0; rr < R; ++rr)
        g.drawText(rows[rr],
            r.getX() + 6,
            (int)(r.getY() + rr * cellH) + 2,
            juce::jmin(110, r.getWidth() / 6),
            (int)cellH - 4,
            juce::Justification::left);

    g.setColour(NoteFill());
    for (const auto& n : pattern)
    {
        const int col = (n.startTick / 24) % C;
        const float x = r.getX() + col * cellW + 2.f;
        const float y = r.getY() + n.row * cellH + 2.f;
        g.fillRoundedRectangle(juce::Rectangle<float>(x, y, cellW - 4.f, cellH - 4.f), 4.f);
    }
}

void DrumGridComponent::mouseDown(const juce::MouseEvent& e)
{
    const int C = 64;
    const int R = juce::jmax(1, rows.size());
    const int headerH = 22;
    auto r = getLocalBounds().withTrimmedTop(headerH);
    const float cellW = r.getWidth() / (float)C;
    const float cellH = r.getHeight() / (float)R;

    if (!r.contains(e.getPosition()))
        return;

    const int c = juce::jlimit(0, C - 1, (int)((e.position.x - r.getX()) / cellW));
    const int rr = juce::jlimit(0, R - 1, (int)((e.position.y - r.getY()) / cellH));

    if (onToggle)
        onToggle(rr, c * 24);
}
