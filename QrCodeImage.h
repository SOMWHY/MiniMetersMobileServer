#pragma once

#include <JuceHeader.h>

class QrCodeImage {
public:
    static juce::Image create(const juce::String& payload, int scale = 5);
};
