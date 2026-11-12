#include "PairingToken.h"

#include <array>
#include <random>

PairingToken::PairingToken(std::chrono::minutes tokenLifetime)
    : lifetime(tokenLifetime) {
    refresh();
}

std::string PairingToken::refresh() {
    std::lock_guard lock(mutex);
    token = makeToken();
    expiresAt = Clock::now() + lifetime;
    return token;
}

std::string PairingToken::current() const {
    std::lock_guard lock(mutex);
    return token;
}

bool PairingToken::validate(const std::string& candidate) const {
    std::lock_guard lock(mutex);
    return !candidate.empty() && candidate == token && Clock::now() < expiresAt;
}

bool PairingToken::expired() const {
    std::lock_guard lock(mutex);
    return Clock::now() >= expiresAt;
}

std::chrono::seconds PairingToken::remaining() const {
    std::lock_guard lock(mutex);
    const auto now = Clock::now();
    if (now >= expiresAt)
        return std::chrono::seconds(0);

    return std::chrono::duration_cast<std::chrono::seconds>(expiresAt - now);
}

std::string PairingToken::makeToken() const {
    static constexpr char alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<std::size_t> distribution(0, sizeof(alphabet) - 2);

    std::string result;
    result.reserve(32);
    for (int i = 0; i < 32; ++i)
        result += alphabet[distribution(generator)];

    return result;
}
