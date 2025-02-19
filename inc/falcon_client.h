#pragma once

#include "falcon.h"
#include "Stream.h"
#include <chrono>
#include <map>

class FalconClient : 
	public Falcon
{
public :
    FalconClient() = default;
    ~FalconClient();
    FalconClient(const FalconClient&) = default;
    FalconClient& operator=(const FalconClient&) = default;
    FalconClient(FalconClient&&) = default;
    FalconClient& operator=(FalconClient&&) = default;

    void ConnectTo(const std::string& ip, uint16_t port) override;
    void OnConnectionEvent(std::function<void(bool, uint64_t)> handler) override;
    void OnDisconnect(std::function<void()> handler) override;

    std::function<void(bool, uint64_t)> m_on_connect = nullptr;
    std::function<void()> m_on_disconnect = nullptr;

    std::unique_ptr<Stream> CreateStream(bool reliable);

    bool IsConnected() const { return m_connected; }

    uint32_t m_lastUsedStreamID = 0;
    uint32_t GetNewStreamID(bool reliable);

    uint64_t GetId() const { return m_id; }

    void SendData(std::span<const char> data, uint32_t stream_id);
private :    
    static void ThreadListen(FalconClient& client);

    IpPortPair server;
    uint64_t m_id;
    std::map<uint32_t, std::unique_ptr<Stream>> m_streams;

    bool m_connected = false;
};