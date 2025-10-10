#pragma once
#include <JuceHeader.h>

namespace boomtheme
{
    inline juce::Colour MainBackground()    { return juce::Colour::fromString("FF7CD400"); }
    inline juce::Colour GridBackground()    { return juce::Colour::fromString("FF092806"); }
    inline juce::Colour GridLine()          { return juce::Colour::fromString("FF2D2E41"); }
    inline juce::Colour HeaderBackground()  { return juce::Colour::fromString("FF6E138B"); }
    inline juce::Colour LightAccent()       { return juce::Colour::fromString("FFC9D2A7"); }
    inline juce::Colour NoteFill()          { return juce::Colour::fromString("FF7CD400"); }
    inline juce::Colour PanelStroke()       { return juce::Colour::fromString("FF3A1484"); }

    inline void drawPanel(juce::Graphics& g, juce::Rectangle<float> r, float radius = 12.f)
    {
        g.setColour(GridBackground()); g.fillRoundedRectangle(r, radius);
        g.setColour(PanelStroke());    g.drawRoundedRectangle(r, radius, 1.5f);
    }
}