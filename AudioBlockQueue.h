#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
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

    const AudioBlock* beginLatestRead() noexcept {
        const auto read = readIndex.load(std::memory_order_relaxed);
        const auto write = writeIndex.load(std::memory_order_acquire);
        if (read == write)
            return nullptr;

        const auto latest = (write + capacity - 1) % capacity;
        readIndex.store(latest, std::memory_order_release);
        return &blocks[latest];
    }

    void endRead() noexcept {
        const auto read = readIndex.load(std::memory_order_relaxed);
        readIndex.store((read + 1) % capacity, std::memory_order_release);
    }

    /**
     * Concatenates every queued block into `destination` and consumes them all.
     *
     * The network thread polls far slower than the audio thread writes (30 Hz vs
     * ~90 blocks/s at 48 kHz), so sending one block per tick would deliver about
     * a third of realtime and the phone could never keep its buffer full.
     * Coalescing keeps the stream at realtime with one message per tick.
     *
     * Returns the number of blocks consumed (0 when the queue is empty).
     * `maxSamplesPerChannel` bounds the destination capacity.
     */
    std::size_t readAll(AudioBlock& destination, std::size_t maxSamplesPerChannel) noexcept {
        destination.channels = 0;
        destination.sampleRate = 0;
        destination.samplesPerChannel = 0;
        destination.timestampMilliseconds = 0;

        std::size_t consumed = 0;
        for (;;) {
            const auto* block = beginRead();
            if (block == nullptr || block->channels == 0)
                break;

            if (destination.channels == 0) {
                destination.channels = block->channels;
                destination.sampleRate = block->sampleRate;
            } else if (destination.channels != block->channels
                       || destination.sampleRate != block->sampleRate) {
                break;
            }

            const std::size_t channels = block->channels;
            // The aggregate is reported in `uint16_t`, so the destination must
            // stop before the count wraps: two 32768-sample blocks fit 65536
            // samples but would report zero. Leave the next block queued.
            const std::size_t aggregate = static_cast<std::size_t>(destination.samplesPerChannel)
                                          + block->samplesPerChannel;
            if (aggregate > maxSamplesPerChannel
                || aggregate > std::numeric_limits<std::uint16_t>::max())
                break;

            std::memcpy(destination.samples.data()
                            + static_cast<std::size_t>(destination.samplesPerChannel) * channels,
                        block->samples.data(),
                        static_cast<std::size_t>(block->samplesPerChannel) * channels * sizeof(float));
            destination.samplesPerChannel = static_cast<std::uint16_t>(aggregate);
            destination.timestampMilliseconds = block->timestampMilliseconds;

            endRead();
            ++consumed;
        }

        return consumed;
    }

private:
    std::array<AudioBlock, capacity> blocks {};
    std::atomic<std::size_t> readIndex { 0 };
    std::atomic<std::size_t> writeIndex { 0 };
};
