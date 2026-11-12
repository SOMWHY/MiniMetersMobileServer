#pragma once

#include <JuceHeader.h>

#include <optional>

class LanAddress {
public:
    static std::optional<juce::String> findFirst();

private:
    static bool isUsableIPv4(const juce::String& address);
};
