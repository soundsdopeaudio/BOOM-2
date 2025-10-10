#pragma once
#include <JuceHeader.h>

namespace boom
{
    enum class Engine : int { e808 = 0, Bass = 1, Drums = 2 };

    inline const juce::StringArray& engineChoices()
    {
        static const juce::StringArray c { "808", "Bass", "Drums" };
        return c;
    }

    inline const juce::StringArray& styleChoices()
    {
        // You asked to include "wxstie" too
        static const juce::StringArray c { "trap", "drill", "hip hop", "rock", "edm", "reggaeton", "r&b", "pop", "wxstie" };
        return c;
    }

    inline const juce::StringArray& keyChoices()
    {
        static const juce::StringArray c { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return c;
    }

    inline const juce::StringArray& scaleChoices()
    {
        // Full BANG list you provided
        static const juce::StringArray c {
            "Major", "Natural Minor", "Harmonic Minor", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian", "Locrian",
                "Locrian Nat6", "Ionian #5", "Dorian #4", "Phrygian Dom", "Lydian #2", "Super Locrian", "Dorian b2",
                "Lydian Aug", "Lydian Dom", "Mixo b6", "Locrian #2", "8 Tone Spanish", "Phrygian Nat3",
                "Blues", "Hungarian Min", "Harmonic Maj(Ethiopian)", "Dorian b5", "Phrygian b4", "Lydian b3", "Mixolydian b2",
                "Lydian Aug2", "Locrian bb7", "Pentatonic Maj", "Pentatonic Min", "Neopolitan Maj",
                "Neopolitan Min", "Spanish Gypsy", "Romanian Minor", "Chromatic", "Bebop Major", "Bebop Minor"
        };
        return c;
    }

    inline const juce::StringArray& timeSigChoices()
    {
        // Full BANG list you provided (including additive meters)
        static const juce::StringArray c {
            "4/4", "3/4", "6/8", "7/8", "5/4", "9/8", "12/8", "2/4", "7/4", "9/4",
                "5/8", "10/8", "11/8", "13/8", "15/8", "17/8", "19/8", "21/8",
                "5/16", "7/16", "9/16", "11/16", "13/16", "15/16", "17/16", "19/16",
                "3+2/8", "2+3/8",
                "2+2+3/8", "3+2+2/8", "2+3+2/8",
                "3+3+2/8", "3+2+3/8", "2+3+3/8",
                "4+3/8", "3+4/8",
                "3+2+2+3/8"
        };
        return c;
    }

    inline const juce::StringArray& barsChoices()
    {
        static const juce::StringArray c { "4", "8" };
        return c;
    }

    inline const juce::StringArray& defaultDrumRows()
    {
        static const juce::StringArray rows { "Kick", "Snare/Clap", "Hi-Hat", "Open Hat", "Perc 1", "Perc 2", "Perc 3" };
        return rows;
    }
}