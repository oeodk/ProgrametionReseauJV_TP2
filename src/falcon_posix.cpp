#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>

#include <memory>
#include <fmt/core.h>
#include "falcon.h"

std::string IpToString(const sockaddr* sa)
{
    switch(sa->sa_family)
    {
    case AF_INET: {
        char ip[INET_ADDRSTRLEN + 6];
        const char* ret = inet_ntop(AF_INET,
            &reinterpret_cast<const sockaddr_in*>(sa)->sin_addr,
            ip,
            INET_ADDRSTRLEN);
        return fmt::format("{}:{}", ret, ntohs(reinterpret_cast<const sockaddr_in*>(sa)->sin_port));
    }
    case AF_INET6: {
        char ip[INET6_ADDRSTRLEN + 8];
        const char* ret = inet_ntop(AF_INET6,
            &reinterpret_cast<const sockaddr_in6*>(sa)->sin6_addr,
            ip+ 1,
            INET6_ADDRSTRLEN);
        return fmt::format("[{}]:{}", ret, ntohs(reinterpret_cast<const sockaddr_in6*>(sa)->sin6_port));
    }
    }

    return "";
}

sockaddr StringToIp(const std::string& ip, uint16_t port)
{
    sockaddr result {};
    int error = inet_pton(AF_INET, ip.c_str(), &result);
    if (error == 1) {
        result.sa_family = AF_INET;
#ifndef __linux__
        result.sa_len = sizeof(sockaddr_in);
#endif
        reinterpret_cast<sockaddr_in*>(&result)->sin_port = htons(port);
        return result;
    }

    memset(&result, 0, sizeof(result));
    error = inet_pton(AF_INET6, ip.c_str(), &result);
    if (error == 1) {
        result.sa_family = AF_INET6;
#ifndef __linux__
        result.sa_len = sizeof(sockaddr_in6);
#endif
        reinterpret_cast<sockaddr_in6*>(&result)->sin6_port = htons(port);
        return result;
    }
    memset(&result, 0, sizeof(result));
    return result;
}

Falcon::Falcon() {

}

Falcon::~Falcon() {
    if(m_socket > 0)
    {
        close(m_socket);
    }
}

void Falcon::CreateServer(uint16_t port)
{
	if(m_socket > 0)
    {
        close(m_socket);
    }
    sockaddr local_endpoint = StringToIp("::", port);
    m_socket = socket(local_endpoint.sa_family,
        SOCK_DGRAM,
        IPPROTO_UDP);
    if (int error = bind(m_socket, &local_endpoint, sizeof(local_endpoint)); error != 0)
    {
        close(m_socket);
    }
}

void Falcon::CreateClient(const std::string& serverIp)
{
	if(m_socket > 0)
    {
        close(m_socket);
    }
    sockaddr local_endpoint = StringToIp(serverIp, 0);
    m_socket = socket(local_endpoint.sa_family,
        SOCK_DGRAM,
        IPPROTO_UDP);
    if (int error = bind(m_socket, &local_endpoint, sizeof(local_endpoint)); error != 0)
    {
        close(m_socket);
    }
}

int Falcon::SendToInternal(const std::string &to, uint16_t port, std::span<const char> message)
{
    const sockaddr destination = StringToIp(to, port);
    int error = sendto(m_socket,
        message.data(),
        message.size(),
        0,
        &destination,
        sizeof(destination));
    return error;
}

int Falcon::ReceiveFromInternal(std::string &from, std::span<char, 65535> message)
{
    struct pollfd fds;
    fds.fd = m_socket;
    fds.events = POLLIN;  // Check for data available to read

    int result = poll(&fds, 1, m_timeout_ms);  
	if (result < 1)
    {
        return 0;  // Timeout
    }
	
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
    const int read_bytes = recvfrom(m_socket,
        message.data(),
        message.size_bytes(),
        0,
        reinterpret_cast<sockaddr*>(&peer_addr),
        &peer_addr_len);

    from = IpToString(reinterpret_cast<const sockaddr*>(&peer_addr));

    return read_bytes;
}