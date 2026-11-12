#pragma once

#include "AudioBlockQueue.h"
#include "PairingToken.h"

#include <JuceHeader.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class WebSocketServer {
public:
    static constexpr int defaultPort = 8422;

    explicit WebSocketServer(AudioBlockQueue& audioQueue, int port = defaultPort);
    ~WebSocketServer();

    bool start();
    void stop();
    bool isRunning() const noexcept;

    PairingToken& pairingToken() noexcept { return token; }
    const PairingToken& pairingToken() const noexcept { return token; }
    int getPort() const noexcept { return port; }

private:
    class ClientSession;

    void acceptLoop();
    void broadcastLoop();
    void addClient(std::shared_ptr<ClientSession> client);

    AudioBlockQueue& audioQueue;
    const int port;
    PairingToken token;
    std::atomic<bool> running { false };
    juce::StreamingSocket listener;
    std::thread acceptThread;
    std::thread broadcastThread;
    std::mutex clientsMutex;
    std::vector<std::shared_ptr<ClientSession>> clients;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebSocketServer)
};
