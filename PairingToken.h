#pragma once

#include <chrono>
#include <mutex>
#include <string>

class PairingToken {
public:
    using Clock = std::chrono::steady_clock;

    explicit PairingToken(std::chrono::minutes lifetime = std::chrono::minutes(10));

    std::string refresh();
    std::string current() const;
    bool validate(const std::string& token) const;
    bool expired() const;
    std::chrono::seconds remaining() const;

private:
    std::string makeToken() const;

    const std::chrono::minutes lifetime;
    mutable std::mutex mutex;
    std::string token;
    Clock::time_point expiresAt {};
};
