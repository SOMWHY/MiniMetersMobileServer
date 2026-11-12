#include "AudioBlockQueue.h"
#include "PairingToken.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>

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
}

int main() {
    testPairingToken();
    testAudioBlockQueue();
    return 0;
}
