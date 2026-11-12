#include "LanAddress.h"

#include <array>

std::optional<juce::String> LanAddress::findFirst() {
    std::optional<juce::String> fallback;
    for (const auto& address : juce::IPAddress::getAllAddresses()) {
        const auto text = address.toString();
        if (!isUsableIPv4(text))
            continue;

        if (isPrivateIPv4(text))
            return text;

        if (!fallback)
            fallback = text;
    }

    return fallback;
}

bool LanAddress::isUsableIPv4(const juce::String& address) {
    const auto parts = juce::StringArray::fromTokens(address, ".", "");
    if (parts.size() != 4)
        return false;

    std::array<int, 4> octets {};
    for (int i = 0; i < 4; ++i) {
        if (! parts[i].containsOnly("0123456789"))
            return false;

        octets[static_cast<std::size_t>(i)] = parts[i].getIntValue();
        if (octets[static_cast<std::size_t>(i)] < 0 || octets[static_cast<std::size_t>(i)] > 255)
            return false;
    }

    if (octets[0] == 0 || octets[0] == 127)
        return false;

    if (octets[0] == 169 && octets[1] == 254)
        return false;

    if (octets[0] == 198 && octets[1] == 18)
        return false;

    if (octets[0] == 192 && octets[1] == 0 && octets[2] == 2)
        return false;

    if (octets[0] == 203 && octets[1] == 0 && octets[2] == 113)
        return false;

    return true;
}

bool LanAddress::isPrivateIPv4(const juce::String& address) {
    const auto parts = juce::StringArray::fromTokens(address, ".", "");
    if (parts.size() != 4)
        return false;

    const auto first = parts[0].getIntValue();
    const auto second = parts[1].getIntValue();
    return first == 10
        || (first == 172 && second >= 16 && second <= 31)
        || (first == 192 && second == 168);
}
