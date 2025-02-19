#pragma once

#include "falcon.h"
#include "Stream.h"
#include <thread>

class FalconServer :
	public Falcon
{
public :
    FalconServer() = default;
    ~FalconServer();
    FalconServer(const FalconServer&) = default;
    FalconServer& operator=(const FalconServer&) = default;
    FalconServer(FalconServer&&) = default;
    FalconServer& operator=(FalconServer&&) = default;

	void Listen(uint16_t port) override;
    virtual void OnClientConnected(std::function<void(uint64_t)> handler) override;
    virtual void OnClientDisconnected(std::function<void(uint64_t)> handler) override;

    std::function<void(uint64_t)> m_on_client_connect = nullptr;
    std::function<void(uint64_t)> m_on_client_disconnect = nullptr;

    uint32_t GetActiveClientCount() const { return m_active_client_count; }

    std::unique_ptr<Stream> CreateStream(uint64_t client, bool reliable);
    void CloseStream(const Stream& stream);

private:
    static void ThreadListen(FalconServer& server);

    std::unordered_map<uint64_t, IpPortPair> m_clients;

    uint64_t m_new_client{};
    uint64_t m_last_disconnected_client{};

    uint32_t m_active_client_count{};
};