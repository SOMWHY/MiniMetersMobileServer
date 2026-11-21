#include "AudioBlockQueue.h"
#include "PairingToken.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace {
void testPairingToken() {
    PairingToken token(std::chrono::minutes(10));
    const auto value = token.current();
    assert(value.size() == 32);
    assert(token.validate(value));
    assert(!token.validate("invalid-token"));
    assert(!token.expired());
    assert(token.remaining().count() > 0);

    const auto refreshed = token.refresh();
    assert(refreshed.size() == 32);
    assert(refreshed != value);
    assert(!token.validate(value));
    assert(token.validate(refreshed));

    PairingToken expired(std::chrono::minutes(0));
    assert(expired.expired());
    assert(!expired.validate(expired.current()));
}

// A phone pairs once with the QR code, then reconnects with the device token it
// was issued: the QR code expiring must not lock it out.
void testDeviceTokens() {
    PairingToken token(std::chrono::minutes(10));
    assert(token.deviceTokenCount() == 0);
    assert(!token.validateDeviceToken(""));
    assert(!token.validateDeviceToken("never-issued"));

    const auto issued = token.issueDeviceToken();
    assert(issued.size() == 32);
    assert(token.deviceTokenCount() == 1);
    assert(token.validateDeviceToken(issued));

    assert(token.authorize("", issued));
    assert(!token.authorize("", "some-other-token"));
    assert(!token.authorize("", ""));

    // The QR token still authorizes on its own.
    PairingToken qrOnly(std::chrono::minutes(10));
    const auto live = qrOnly.current();
    assert(qrOnly.authorize(live, ""));

    // ...and so does the token that was just rotated out, inside the grace
    // window, because a scan can land a moment after the refresh.
    const auto rotated = qrOnly.refresh();
    assert(qrOnly.authorize(live, ""));
    assert(qrOnly.authorize(rotated, ""));
    assert(!qrOnly.authorize("", ""));

    token.forgetDeviceTokens();
    assert(token.deviceTokenCount() == 0);
    assert(!token.validateDeviceToken(issued));
}

void testDeviceTokenPersistence() {
    // A unique directory per invocation: concurrent test executables must not
    // delete each other's token file mid-assertion.
    const auto marker = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto directory = std::filesystem::temp_directory_path() / ("minimeters_tokens_" + marker);
    std::filesystem::create_directories(directory);
    const auto path = directory / "device_tokens.txt";
    std::filesystem::remove(path);
    const auto storePath = path.string();

    std::string issued;

    {
        PairingToken token(std::chrono::minutes(10), storePath);
        issued = token.issueDeviceToken();
    }

    {
        PairingToken reopened(std::chrono::minutes(10), storePath);
        assert(reopened.deviceTokenCount() == 1);
        assert(reopened.validateDeviceToken(issued));
        assert(reopened.authorize("", issued));
        reopened.forgetDeviceTokens();
        assert(reopened.deviceTokenCount() == 0);
    }

    {
        PairingToken cleared(std::chrono::minutes(10), storePath);
        assert(cleared.deviceTokenCount() == 0);
    }

    std::filesystem::remove_all(directory);
}

void testAudioBlockQueue() {
    auto queue = std::make_unique<AudioBlockQueue>();
    float samples[4] { 0.1f, -0.2f, 0.3f, -0.4f };
    assert(queue->tryWrite(samples, 4, 48000, 2, 2, 123));

    const auto* block = queue->beginRead();
    assert(block != nullptr);
    assert(block->sampleRate == 48000);
    assert(block->channels == 2);
    assert(block->samplesPerChannel == 2);
    assert(block->timestampMilliseconds == 123);
    assert(block->samples[0] == samples[0]);
    assert(block->samples[3] == samples[3]);
    queue->endRead();
    assert(queue->beginRead() == nullptr);
}

// The network thread polls slower than the audio thread writes, so every queued
// block has to be sent in one message or the stream runs below realtime.
void testAudioBlockCoalescing() {
    // AudioBlock is 512 kB and AudioBlockQueue holds four of them, so both live
    // on the heap: they do not fit in the default 1 MB stack.
    auto queue = std::make_unique<AudioBlockQueue>();
    const float first[2] { 0.5f, -0.5f };
    const float second[2] { 0.25f, -0.25f };
    assert(queue->tryWrite(first, 2, 48000, 2, 1, 10));
    assert(queue->tryWrite(second, 2, 48000, 2, 1, 20));

    auto merged = std::make_unique<AudioBlock>();
    assert(queue->readAll(*merged, AudioBlock::maxSamplesPerChannel) == 2);
    assert(merged->channels == 2);
    assert(merged->sampleRate == 48000);
    assert(merged->samplesPerChannel == 2);
    assert(merged->timestampMilliseconds == 20);
    assert(merged->samples[0] == 0.5f);
    assert(merged->samples[1] == -0.5f);
    assert(merged->samples[2] == 0.25f);
    assert(merged->samples[3] == -0.25f);
    assert(queue->beginRead() == nullptr);

    auto empty = std::make_unique<AudioBlock>();
    assert(queue->readAll(*empty, AudioBlock::maxSamplesPerChannel) == 0);
    assert(empty->channels == 0);
    assert(empty->samplesPerChannel == 0);
}
void testAudioBlockAggregateCap() {
    auto queue = std::make_unique<AudioBlockQueue>();
    constexpr std::size_t perChannel = 32768;

    auto block = std::make_unique<std::vector<float>>(perChannel * 2, 0.5f);
    assert(queue->tryWrite(block->data(), block->size(), 48000, 2,
                           static_cast<std::uint16_t>(perChannel), 10));
    assert(queue->tryWrite(block->data(), block->size(), 48000, 2,
                           static_cast<std::uint16_t>(perChannel), 20));

    // 32768 + 32768 fits the 65536-sample destination but would report zero in
    // a uint16_t, so the second block has to stay queued.
    auto merged = std::make_unique<AudioBlock>();
    assert(queue->readAll(*merged, AudioBlock::maxSamplesPerChannel) == 1);
    assert(merged->samplesPerChannel == perChannel);

    auto rest = std::make_unique<AudioBlock>();
    assert(queue->readAll(*rest, AudioBlock::maxSamplesPerChannel) == 1);
    assert(rest->samplesPerChannel == perChannel);
    assert(queue->beginRead() == nullptr);
}

} // namespace

int main() {
    testPairingToken();
    testDeviceTokens();
    testDeviceTokenPersistence();
    testAudioBlockQueue();
    testAudioBlockCoalescing();
    testAudioBlockAggregateCap();
    return 0;
}
