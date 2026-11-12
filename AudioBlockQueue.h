#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

struct AudioBlock {
    static constexpr std::size_t maxSamplesPerChannel = 65536;
    static constexpr std::size_t maxChannels = 2;
    static constexpr std::size_t maxSamples = maxSamplesPerChannel * maxChannels;

    std::array<float, maxSamples> samples {};
    std::uint64_t timestampMilliseconds = 0;
    std::uint32_t sampleRate = 0;
    std::uint16_t channels = 0;
    std::uint16_t samplesPerChannel = 0;
};

class AudioBlockQueue {
public:
    static constexpr std::size_t capacity = 4;

    bool tryWrite(const float* source,
                  std::size_t sampleCount,
                  std::uint32_t sampleRate,
                  std::uint16_t channels,
                  std::uint16_t samplesPerChannel,
                  std::uint64_t timestampMilliseconds) noexcept {
        if (source == nullptr || sampleCount > AudioBlock::maxSamples
            || channels == 0 || channels > AudioBlock::maxChannels
            || samplesPerChannel == 0
            || samplesPerChannel > std::numeric_limits<std::uint16_t>::max())
            return false;

        const auto write = writeIndex.load(std::memory_order_relaxed);
        const auto next = (write + 1) % capacity;
        if (next == readIndex.load(std::memory_order_acquire))
            return false;

        auto& block = blocks[write];
        for (std::size_t i = 0; i < sampleCount; ++i)
            block.samples[i] = source[i];

        block.timestampMilliseconds = timestampMilliseconds;
        block.sampleRate = sampleRate;
        block.channels = channels;
        block.samplesPerChannel = samplesPerChannel;
        writeIndex.store(next, std::memory_order_release);
        return true;
    }

    const AudioBlock* beginRead() noexcept {
        const auto read = readIndex.load(std::memory_order_relaxed);
        if (read == writeIndex.load(std::memory_order_acquire))
            return nullptr;
        return &blocks[read];
    }

    void endRead() noexcept {
        const auto read = readIndex.load(std::memory_order_relaxed);
        readIndex.store((read + 1) % capacity, std::memory_order_release);
    }

private:
    std::array<AudioBlock, capacity> blocks {};
    std::atomic<std::size_t> readIndex { 0 };
    std::atomic<std::size_t> writeIndex { 0 };
};
