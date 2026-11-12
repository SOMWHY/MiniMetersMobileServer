#include "LanAddress.h"

#include <array>

std::optional<juce::String> LanAddress::findFirst() {
    for (const auto& address : juce::IPAddress::getAllAddresses()) {
        const auto text = address.toString();
        if (isUsableIPv4(text))
            return text;
    }

    return std::nullopt;
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

    return true;
}
