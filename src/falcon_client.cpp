#include "falcon_client.h"
#include "message_type.h"
#include <array>

using namespace std::chrono_literals;

constexpr std::chrono::microseconds TIMEOUT = 1000ms;

FalconClient::~FalconClient()
{
	m_listen = true;
	m_listener.join();
}

void FalconClient::ConnectTo(const std::string& ip, uint16_t port)
{
	Falcon::CreateClient(ip);
	m_listen = true;
	m_listener = std::thread(ThreadListen, std::ref(*this));
	server.ip = ip;
	server.port = port;

	std::string connection_message;
	const uint32_t msg_size = 6;
	connection_message.resize(msg_size);

	connection_message[0] = CONNECT;
	memcpy(&connection_message[1], &msg_size, sizeof(msg_size));
	memcpy(&connection_message[5], &m_version, sizeof(m_version));

	SendTo(server.ip, server.port, connection_message);
}

void FalconClient::OnConnectionEvent(std::function<void(bool, uint64_t)> handler)
{
	handler(m_connected, m_id);
}

void FalconClient::OnDisconnect(std::function<void()> handler)
{
	handler();
}

void FalconClient::ThreadListen(FalconClient& client)
{
	while(client.m_listen)
	{
		std::array<char, 65535> buffer;
		std::string other_ip;
		int recv_size = client.ReceiveFrom(other_ip, buffer);
		printf("test");
		if(client.m_connected)
		{
			if (recv_size != 0)
			{
				switch (MessageType(buffer[0]))
				{
				case CONNECT_ACK:
				{
					client.m_connected = true;
					memcpy(&client.m_id, &buffer[5], sizeof(client.m_id));
					if (client.m_on_connect != nullptr)
					{
						client.OnConnectionEvent(client.m_on_connect);
					}
				}
				break;
				case DISCONNECT:
					if (client.m_on_disconnect != nullptr)
					{
						client.OnDisconnect(client.m_on_disconnect);
					}
					break;
				default:
					//stream
					break;
				}
			}
		}
		else
		{
			client.OnConnectionEvent(client.m_on_connect);
		}
	}
}
