#include "WebSocketServer.h"

#include "json.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

#if JUCE_WINDOWS
 #include <winsock2.h>
#else
 #include <netinet/in.h>
 #include <netinet/tcp.h>
#endif

namespace {
using json = nlohmann::json;

/**
 * Stop the stack from batching our writes.
 *
 * This JUCE build predates `StreamingSocket::setNoDelay`, so the option goes on
 * the raw handle. Nagle holds a small write until the previous one is ACKed; on
 * a 100 Hz audio stream of ~2 KB messages that is tens of milliseconds of extra
 * latency per packet, and the listener hears it as the meters trailing the sound.
 * Each socket has a single writer, so there is nothing worth coalescing.
 */
bool disableNagleDelay(juce::StreamingSocket& socket) {
    const auto handle = socket.getRawSocketHandle();
    if (handle < 0)
        return false;

    const int enabled = 1;
#if JUCE_WINDOWS
    return ::setsockopt(handle, IPPROTO_TCP, TCP_NODELAY,
                        reinterpret_cast<const char*>(&enabled), sizeof(enabled)) == 0;
#else
    return ::setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled)) == 0;
#endif
}

std::uint32_t rotateLeft(std::uint32_t value, int bits) {
    return (value << bits) | (value >> (32 - bits));
}

std::array<std::uint8_t, 20> sha1(std::string_view input) {
    std::vector<std::uint8_t> message(input.begin(), input.end());
    const auto bitLength = static_cast<std::uint64_t>(message.size()) * 8;
    message.push_back(0x80);
    while ((message.size() % 64) != 56)
        message.push_back(0);
    for (int i = 7; i >= 0; --i)
        message.push_back(static_cast<std::uint8_t>(bitLength >> (i * 8)));

    std::uint32_t h0 = 0x67452301;
    std::uint32_t h1 = 0xefcdab89;
    std::uint32_t h2 = 0x98badcfe;
    std::uint32_t h3 = 0x10325476;
    std::uint32_t h4 = 0xc3d2e1f0;

    for (std::size_t offset = 0; offset < message.size(); offset += 64) {
        std::array<std::uint32_t, 80> words {};
        for (int i = 0; i < 16; ++i) {
            const auto index = offset + static_cast<std::size_t>(i) * 4;
            words[static_cast<std::size_t>(i)] = (static_cast<std::uint32_t>(message[index]) << 24)
                | (static_cast<std::uint32_t>(message[index + 1]) << 16)
                | (static_cast<std::uint32_t>(message[index + 2]) << 8)
                | static_cast<std::uint32_t>(message[index + 3]);
        }
        for (int i = 16; i < 80; ++i)
            words[static_cast<std::size_t>(i)] = rotateLeft(words[static_cast<std::size_t>(i - 3)]
                                                               ^ words[static_cast<std::size_t>(i - 8)]
                                                               ^ words[static_cast<std::size_t>(i - 14)]
                                                               ^ words[static_cast<std::size_t>(i - 16)],
                                                           1);

        auto a = h0;
        auto b = h1;
        auto c = h2;
        auto d = h3;
        auto e = h4;
        for (int i = 0; i < 80; ++i) {
            std::uint32_t function = 0;
            std::uint32_t constant = 0;
            if (i < 20) {
                function = (b & c) | ((~b) & d);
                constant = 0x5a827999;
            } else if (i < 40) {
                function = b ^ c ^ d;
                constant = 0x6ed9eba1;
            } else if (i < 60) {
                function = (b & c) | (b & d) | (c & d);
                constant = 0x8f1bbcdc;
            } else {
                function = b ^ c ^ d;
                constant = 0xca62c1d6;
            }
            const auto temp = rotateLeft(a, 5) + function + e + constant + words[static_cast<std::size_t>(i)];
            e = d;
            d = c;
            c = rotateLeft(b, 30);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::array<std::uint8_t, 20> result {};
    const std::array<std::uint32_t, 5> words { h0, h1, h2, h3, h4 };
    for (std::size_t i = 0; i < words.size(); ++i)
        for (int j = 0; j < 4; ++j)
            result[i * 4 + static_cast<std::size_t>(j)] = static_cast<std::uint8_t>(words[i] >> ((3 - j) * 8));
    return result;
}

std::string base64(const std::uint8_t* data, std::size_t size) {
    static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve((size + 2) / 3 * 4);
    for (std::size_t i = 0; i < size; i += 3) {
        const auto first = data[i];
        const auto second = i + 1 < size ? data[i + 1] : 0;
        const auto third = i + 2 < size ? data[i + 2] : 0;
        result += alphabet[first >> 2];
        result += alphabet[((first & 0x03) << 4) | (second >> 4)];
        result += i + 1 < size ? alphabet[((second & 0x0f) << 2) | (third >> 6)] : '=';
        result += i + 2 < size ? alphabet[third & 0x3f] : '=';
    }
    return result;
}

std::uint64_t nowMilliseconds() {
    return static_cast<std::uint64_t>(juce::Time::currentTimeMillis());
}
}

class WebSocketServer::ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(std::unique_ptr<juce::StreamingSocket> client, PairingToken& pairingToken)
        : socket(std::move(client)), token(pairingToken)
    {
        if (socket != nullptr)
            disableNagleDelay(*socket);
    }

    ~ClientSession() { stopAndJoin(); }

    void start() { worker = std::thread([this] { run(); }); }

    void stopAndJoin() {
        stopping.store(true, std::memory_order_release);
        if (socket)
            socket->close();
        if (worker.joinable())
            worker.join();
    }

    bool isAuthenticated() const noexcept { return authenticated.load(std::memory_order_acquire); }
    bool isStopped() const noexcept { return stopping.load(std::memory_order_acquire); }

    bool sendBinary(const AudioBlock& block) {
        std::lock_guard lock(writeMutex);
        if (stopping.load(std::memory_order_acquire) || !authenticated.load(std::memory_order_acquire))
            return false;

        std::vector<std::uint8_t> payload(32 + static_cast<std::size_t>(block.channels) * block.samplesPerChannel * sizeof(float));
        std::memcpy(payload.data(), "MMSA", 4);
        payload[4] = 1;
        payload[5] = 1;
        writeU16(payload.data() + 6, 32);
        writeU64(payload.data() + 8,
                 block.timestampMilliseconds != 0 ? block.timestampMilliseconds : nowMilliseconds());
        writeU32(payload.data() + 16, block.sampleRate);
        writeU16(payload.data() + 20, block.channels);
        writeU16(payload.data() + 22, block.samplesPerChannel);
        writeU32(payload.data() + 24, 1);
        writeU32(payload.data() + 28, static_cast<std::uint32_t>(payload.size() - 32));
        std::memcpy(payload.data() + 32, block.samples.data(), payload.size() - 32);
        const auto sent = sendFrame(0x2, payload.data(), payload.size());
        if (!sent) {
            stopping.store(true, std::memory_order_release);
            socket->close();
        }
        return sent;
    }

    bool sendControl(const std::string& text) {
        if (stopping.load(std::memory_order_acquire) || !authenticated.load(std::memory_order_acquire))
            return false;
        return sendText(text);
    }

private:
    static void writeU16(std::uint8_t* destination, std::uint16_t value) {
        destination[0] = static_cast<std::uint8_t>(value);
        destination[1] = static_cast<std::uint8_t>(value >> 8);
    }

    static void writeU32(std::uint8_t* destination, std::uint32_t value) {
        for (int i = 0; i < 4; ++i)
            destination[i] = static_cast<std::uint8_t>(value >> (i * 8));
    }

    static void writeU64(std::uint8_t* destination, std::uint64_t value) {
        for (int i = 0; i < 8; ++i)
            destination[i] = static_cast<std::uint8_t>(value >> (i * 8));
    }

    bool writeAll(const void* source, int bytes) {
        auto* data = static_cast<const std::uint8_t*>(source);
        while (bytes > 0) {
            const auto written = socket->write(data, bytes);
            if (written <= 0)
                return false;
            data += written;
            bytes -= written;
        }
        return true;
    }

    bool sendFrame(std::uint8_t opcode, const void* data, std::size_t size) {
        std::vector<std::uint8_t> header;
        header.push_back(static_cast<std::uint8_t>(0x80 | opcode));
        if (size < 126) {
            header.push_back(static_cast<std::uint8_t>(size));
        } else if (size <= 0xffff) {
            header.push_back(126);
            header.push_back(static_cast<std::uint8_t>(size >> 8));
            header.push_back(static_cast<std::uint8_t>(size));
        } else {
            header.push_back(127);
            for (int i = 7; i >= 0; --i)
                header.push_back(static_cast<std::uint8_t>(size >> (i * 8)));
        }
        return writeAll(header.data(), static_cast<int>(header.size()))
            && writeAll(data, static_cast<int>(size));
    }

    bool sendText(const std::string& text) {
        std::lock_guard lock(writeMutex);
        return sendFrame(0x1, text.data(), text.size());
    }

    bool readExact(void* destination, int bytes) {
        auto* data = static_cast<std::uint8_t*>(destination);
        while (bytes > 0 && !stopping.load(std::memory_order_acquire)) {
            if (!pendingBytes.empty()) {
                const auto amount = std::min<std::size_t>(pendingBytes.size(), static_cast<std::size_t>(bytes));
                std::memcpy(data, pendingBytes.data(), amount);
                pendingBytes.erase(0, amount);
                data += amount;
                bytes -= static_cast<int>(amount);
                continue;
            }
            if (socket->waitUntilReady(true, 1000) <= 0)
                continue;
            const auto read = socket->read(data, bytes, false);
            if (read < 0)
                return false;
            if (read == 0)
                continue;
            data += read;
            bytes -= read;
        }
        return bytes == 0;
    }

    bool handshake() {
        std::string request;
        std::array<char, 1024> buffer {};
        while (request.find("\r\n\r\n") == std::string::npos && request.size() < 8192) {
            if (socket->waitUntilReady(true, 2000) <= 0)
                return false;
            const auto read = socket->read(buffer.data(), static_cast<int>(buffer.size()), false);
            if (read <= 0)
                return false;
            request.append(buffer.data(), static_cast<std::size_t>(read));
        }

        if (request.rfind("GET /ws ", 0) != 0)
            return false;

        const auto headerEnd = request.find("\r\n\r\n");
        pendingBytes = request.substr(headerEnd + 4);

        const std::string keyName = "Sec-WebSocket-Key:";
        const auto keyPosition = request.find(keyName);
        if (keyPosition == std::string::npos)
            return false;
        const auto keyStart = request.find_first_not_of(" \t", keyPosition + keyName.size());
        const auto keyEnd = request.find("\r\n", keyStart);
        if (keyStart == std::string::npos || keyEnd == std::string::npos)
            return false;
        const auto key = request.substr(keyStart, keyEnd - keyStart);
        const auto digest = sha1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        const auto accept = base64(digest.data(), digest.size());
        const std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                                     "Upgrade: websocket\r\n"
                                     "Connection: Upgrade\r\n"
                                     "Sec-WebSocket-Accept: "
            + accept + "\r\n\r\n";
        return writeAll(response.data(), static_cast<int>(response.size()));
    }

    bool receiveControl() {
        std::uint8_t header[2] {};
        if (!readExact(header, 2))
            return false;
        const auto opcode = static_cast<std::uint8_t>(header[0] & 0x0f);
        const bool masked = (header[1] & 0x80) != 0;
        std::uint64_t length = header[1] & 0x7f;
        if (length == 126) {
            std::uint8_t extended[2] {};
            if (!readExact(extended, 2))
                return false;
            length = (static_cast<std::uint64_t>(extended[0]) << 8) | extended[1];
        } else if (length == 127) {
            std::uint8_t extended[8] {};
            if (!readExact(extended, 8))
                return false;
            length = 0;
            for (auto byte : extended)
                length = (length << 8) | byte;
        }
        if (!masked || length > 4096)
            return false;

        std::array<std::uint8_t, 4> mask {};
        if (!readExact(mask.data(), 4))
            return false;
        std::vector<std::uint8_t> payload(static_cast<std::size_t>(length));
        if (!payload.empty() && !readExact(payload.data(), static_cast<int>(payload.size())))
            return false;
        for (std::size_t i = 0; i < payload.size(); ++i)
            payload[i] ^= mask[i % 4];

        if (opcode == 0x8)
            return false;
        if (opcode == 0x9) {
            std::lock_guard lock(writeMutex);
            return sendFrame(0xA, payload.data(), payload.size());
        }
        if (opcode != 0x1)
            return true;

        try {
            const auto message = json::parse(payload.begin(), payload.end());
            if (message.value("type", "") != "authenticate")
                return false;

            const auto qrToken = message.value("token", std::string {});
            const auto deviceToken = message.value("deviceToken", std::string {});

            if (message.value("version", 0) != 1 || !token.authorize(qrToken, deviceToken)) {
                sendText(R"({"type":"error","code":"invalid_token"})");
                return false;
            }

            // A phone that scanned the QR code gets its own long-lived
            // credential; a returning phone already presents one and keeps it.
            auto issuedDeviceToken = deviceToken;
            if (!token.validateDeviceToken(issuedDeviceToken))
                issuedDeviceToken = token.issueDeviceToken();

            authenticated.store(true, std::memory_order_release);
            sendText(json {
                { "type", "authenticated" },
                { "version", 1 },
                { "sampleFormat", "f32le" },
                { "deviceToken", issuedDeviceToken },
                { "heartbeatIntervalMs", 5000 },
            }.dump());
            return true;
        } catch (...) {
            return false;
        }
    }

    void run() {
        if (!handshake()) {
            socket->close();
            return;
        }
        const auto authenticationDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!stopping.load(std::memory_order_acquire)) {
            if (!authenticated.load(std::memory_order_acquire)
                && std::chrono::steady_clock::now() >= authenticationDeadline)
                break;
            if (!receiveControl())
                break;
        }
        socket->close();
    }

    std::unique_ptr<juce::StreamingSocket> socket;
    PairingToken& token;
    std::atomic<bool> stopping { false };
    std::atomic<bool> authenticated { false };
    std::mutex writeMutex;
    std::thread worker;
    std::string pendingBytes;
};

WebSocketServer::WebSocketServer(AudioBlockQueue& queue, int serverPort, std::string deviceStorePath)
    : audioQueue(queue)
    , port(serverPort)
    , token(PairingToken(std::chrono::minutes(10), std::move(deviceStorePath))) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
    if (running.exchange(true, std::memory_order_acq_rel))
        return true;

    if (!listener.createListener(port)) {
        running.store(false, std::memory_order_release);
        return false;
    }

    acceptThread = std::thread([this] { acceptLoop(); });
    broadcastThread = std::thread([this] { broadcastLoop(); });
    return true;
}

void WebSocketServer::stop() {
    if (!running.exchange(false, std::memory_order_acq_rel))
        return;

    listener.close();
    if (acceptThread.joinable())
        acceptThread.join();
    if (broadcastThread.joinable())
        broadcastThread.join();

    std::lock_guard lock(clientsMutex);
    for (auto& client : clients)
        client->stopAndJoin();
    clients.clear();
}

bool WebSocketServer::isRunning() const noexcept {
    return running.load(std::memory_order_acquire);
}

std::size_t WebSocketServer::connectedClientCount() const {
    std::lock_guard lock(clientsMutex);
    std::size_t count = 0;
    for (const auto& client : clients)
        if (client->isAuthenticated() && !client->isStopped())
            ++count;
    return count;
}

void WebSocketServer::acceptLoop() {
    while (running.load(std::memory_order_acquire)) {
        std::unique_ptr<juce::StreamingSocket> client(listener.waitForNextConnection());
        if (client != nullptr && running.load(std::memory_order_acquire))
            addClient(std::make_shared<ClientSession>(std::move(client), token));
    }
}

void WebSocketServer::broadcastLoop() {
    // The audio thread pushes a block roughly every 10 ms; polling at 10 ms keeps
    // the latency of one block while coalescing everything that piled up.
    constexpr auto broadcastInterval = std::chrono::milliseconds(10);
    constexpr auto heartbeatInterval = std::chrono::milliseconds(5000);

    auto nextBroadcast = std::chrono::steady_clock::now();
    auto nextHeartbeat = std::chrono::steady_clock::now() + heartbeatInterval;

    while (running.load(std::memory_order_acquire)) {
        const auto now = std::chrono::steady_clock::now();
        if (now < nextBroadcast) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        const auto blockCount = audioQueue.readAll(coalescedBlock, AudioBlock::maxSamplesPerChannel);

        if (blockCount == 0) {
            // Silence on the wire used to be indistinguishable from a dead link;
            // a heartbeat lets the phone notice a broken socket within seconds.
            if (now >= nextHeartbeat) {
                const auto heartbeat = json {
                    { "type", "heartbeat" },
                    { "version", 1 },
                    { "timestampMs", nowMilliseconds() },
                }.dump();

                std::lock_guard lock(clientsMutex);
                for (auto& client : clients)
                    client->sendControl(heartbeat);
                nextHeartbeat = now + heartbeatInterval;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        std::lock_guard lock(clientsMutex);
        for (auto& client : clients)
            if (client->isAuthenticated())
                client->sendBinary(coalescedBlock);
        clients.erase(std::remove_if(clients.begin(), clients.end(), [](const auto& client) {
                          return client->isStopped();
                      }),
                      clients.end());

        nextBroadcast = std::chrono::steady_clock::now() + broadcastInterval;
        if (nextHeartbeat < nextBroadcast)
            nextHeartbeat = nextBroadcast + heartbeatInterval;
    }
}

void WebSocketServer::addClient(std::shared_ptr<ClientSession> client) {
    {
        std::lock_guard lock(clientsMutex);
        clients.push_back(client);
    }
    client->start();
}
