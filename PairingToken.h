#pragma once

#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

/**
 * Two kinds of credentials guard the stream:
 *
 *  - the short-lived **QR token**, printed in the editor and scanned once;
 *  - a long-lived **device token** the phone receives on its first successful
 *    handshake and replays on every later connect.
 *
 * The QR token expiring must never kick a phone out of a plugin it has already
 * paired with, so the device token is what a reconnecting phone presents.
 * Device tokens survive plugin restarts via an optional small text file.
 */
class PairingToken {
public:
    using Clock = std::chrono::steady_clock;

    static constexpr std::size_t maxDeviceTokens = 16;

    explicit PairingToken(std::chrono::minutes lifetime = std::chrono::minutes(10),
                          std::string deviceStorePath = {});

    // --- QR token -----------------------------------------------------------
    std::string refresh();
    std::string current() const;
    bool validate(const std::string& token) const;
    bool expired() const;
    std::chrono::seconds remaining() const;

    // --- device tokens ------------------------------------------------------
    /** Mints a fresh device token, stores it, and returns it. */
    std::string issueDeviceToken();
    bool validateDeviceToken(const std::string& candidate) const;
    std::size_t deviceTokenCount() const;
    /** Forces every phone to scan a new QR code. */
    void forgetDeviceTokens();

    /**
     * Accepts a device token, a live QR token, or a QR token that expired less
     * than `grace` ago (a scan that started just before expiry still works).
     */
    bool authorize(const std::string& qrToken, const std::string& deviceToken) const;

private:
    std::string makeToken() const;
    void loadDeviceTokens();
    void persistDeviceTokens(const std::vector<std::string>& tokens) const;

    const std::chrono::minutes lifetime;
    static constexpr std::chrono::seconds grace { 120 };
    const std::string storePath;

    mutable std::mutex mutex;
    std::string token;
    Clock::time_point expiresAt {};
    std::string previousToken;
    Clock::time_point previousExpiresAt {};
    std::vector<std::string> deviceTokens;
};
