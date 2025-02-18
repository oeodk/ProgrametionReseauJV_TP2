#pragma once

#include "falcon.h"
#include <chrono>
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

private :    
    static void ThreadListen(FalconClient& client);

    IpPortPair server;
    uint64_t m_id;

    bool m_connected = false;

};