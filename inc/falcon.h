#pragma once

#include <memory>
#include <string>
#include <span>
#include <functional>
#include <thread>
#include <cstring>

#ifdef WIN32
    using SocketType = unsigned int;
#else
    using SocketType = int;
#endif

struct IpPortPair
{
    std::string ip;
    uint16_t port;
};

class Falcon {
public:


    Falcon();
    virtual ~Falcon();
    Falcon(const Falcon&) = default;
    Falcon& operator=(const Falcon&) = default;
    Falcon(Falcon&&) = default;
    Falcon& operator=(Falcon&&) = default;

    const uint8_t m_version = 1;

    bool IsListening() const { 
        return m_listen;
    }
    int SendTo(const std::string& to, uint16_t port, std::span<const char> message);
    int ReceiveFrom(std::string& from, std::span<char, 65535> message);
protected:

    virtual void CreateServer(uint16_t port);
    virtual void CreateClient(const std::string& ip);

    std::thread m_listener;
    bool m_listen = false;

    SocketType m_socket{};

    int m_timeout_ms = 100;
private:
    virtual void Listen(uint16_t port) {}
    virtual void OnClientConnected(std::function<void(uint64_t)> handler) {}
   
    virtual void ConnectTo(const std::string& ip, uint16_t port) {}
    virtual void OnConnectionEvent(std::function<void(bool, uint64_t)> handler) {}

    virtual void OnClientDisconnected(std::function<void(uint64_t)> handler) {} //Server API 
    virtual void OnDisconnect(std::function<void()> handler) {}  //Client API
    
    int SendToInternal(const std::string& to, uint16_t port, std::span<const char> message);
    int ReceiveFromInternal(std::string& from, std::span<char, 65535> message);

};
