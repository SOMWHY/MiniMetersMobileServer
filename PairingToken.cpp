#include "PairingToken.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <random>

PairingToken::PairingToken(std::chrono::minutes tokenLifetime, std::string deviceStorePath)
    : lifetime(tokenLifetime)
    , storePath(std::move(deviceStorePath)) {
    loadDeviceTokens();
    refresh();
}

std::string PairingToken::refresh() {
    std::lock_guard lock(mutex);
    // Keep the outgoing token valid for a short grace period: a phone that
    // scanned the QR a moment before the refresh should still get in.
    previousToken = token;
    previousExpiresAt = expiresAt;
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

std::string PairingToken::issueDeviceToken() {
    std::lock_guard lock(mutex);
    auto issued = makeToken();
    deviceTokens.push_back(issued);
    // Oldest first: a phone that has not connected in ages is the one to drop.
    if (deviceTokens.size() > maxDeviceTokens)
        deviceTokens.erase(deviceTokens.begin());

    persistDeviceTokens(deviceTokens);
    return issued;
}

bool PairingToken::validateDeviceToken(const std::string& candidate) const {
    if (candidate.empty())
        return false;

    std::lock_guard lock(mutex);
    return std::find(deviceTokens.begin(), deviceTokens.end(), candidate) != deviceTokens.end();
}

std::size_t PairingToken::deviceTokenCount() const {
    std::lock_guard lock(mutex);
    return deviceTokens.size();
}

void PairingToken::forgetDeviceTokens() {
    std::lock_guard lock(mutex);
    deviceTokens.clear();
    persistDeviceTokens(deviceTokens);
}

bool PairingToken::authorize(const std::string& qrToken, const std::string& deviceToken) const {
    if (validateDeviceToken(deviceToken))
        return true;

    std::lock_guard lock(mutex);
    const auto now = Clock::now();
    if (!qrToken.empty() && qrToken == token && now < expiresAt)
        return true;

    // Grace window for the token that just rotated.
    return !qrToken.empty() && qrToken == previousToken
        && now < previousExpiresAt + grace;
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

void PairingToken::loadDeviceTokens() {
    deviceTokens.clear();
    if (storePath.empty())
        return;

    std::ifstream stream(storePath);
    std::string line;
    while (std::getline(stream, line) && deviceTokens.size() < maxDeviceTokens) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (!line.empty())
            deviceTokens.push_back(line);
    }
}

void PairingToken::persistDeviceTokens(const std::vector<std::string>& tokens) const {
    if (storePath.empty())
        return;

    // Failures are silent on purpose: a read-only data directory must not stop
    // the plugin from streaming, it only means phones re-scan after a restart.
    std::ofstream stream(storePath, std::ios::trunc);
    if (!stream)
        return;

    for (const auto& entry : tokens)
        stream << entry << '\n';
}
