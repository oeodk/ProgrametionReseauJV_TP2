#include "falcon_server.h"
#include "message_type.h"
#include <array>
#include "spdlog/spdlog.h"

inline static uint64_t usable_id = 0;

FalconServer::~FalconServer()
{
	m_listen = false;
	m_listener.join();
}

void FalconServer::Listen(uint16_t port)
{
	Falcon::CreateServer(port);
	m_listen = true;
	m_listener = std::thread(ThreadListen, std::ref(*this));
}

void FalconServer::OnClientConnected(std::function<void(uint64_t)> handler)
{
	handler(m_new_client);
}

void FalconServer::OnClientDisconnected(std::function<void(uint64_t)> handler)
{
	handler(m_last_disconnected_client);
}

void FalconServer::ThreadListen(FalconServer& server)
{
	while (server.m_listen)
	{
		std::array<char, 65535> buffer;
		std::string other_ip;
		int recv_size = server.ReceiveFrom(other_ip, buffer);
		if (recv_size != 0)
		{
			spdlog::debug(buffer[0]);
			switch (MessageType(buffer[0]))
			{
			case CONNECT:
			{
				std::string ip = other_ip;
				uint16_t port = 0;
				auto pos = other_ip.find_last_of(':');
				if (pos != std::string::npos)
				{
					ip = other_ip.substr(0, pos);
					std::string port_str = other_ip.substr(++pos);
					port = atoi(port_str.c_str());
				}				
				server.m_new_client = usable_id++;

				server.m_clients.insert({ server.m_new_client , {ip, port} });

				std::string ack_message;
				const uint32_t msg_size = 14;
				ack_message.resize(msg_size);

				ack_message[0] = CONNECT_ACK;
				memcpy(&ack_message[1], &msg_size, sizeof(msg_size));
				memcpy(&ack_message[5], &server.m_new_client, sizeof(server.m_new_client));
				memcpy(&ack_message[13], &server.m_version, sizeof(server.m_version));

				server.SendTo(server.m_clients.at(server.m_new_client).ip, server.m_clients.at(server.m_new_client).port, ack_message);
				if (server.m_on_client_connect != nullptr)
				{
					server.OnClientConnected(server.m_on_client_connect);
				}
			}
				break;
			case DISCONNECT:
				std::memcpy(&server.m_last_disconnected_client, &buffer[5], sizeof(server.m_last_disconnected_client));
				if (server.m_on_client_disconnect != nullptr)
				{
					server.OnClientDisconnected(server.m_on_client_disconnect);
				}
				break;
			default:
				//stream
				break;
			}
		}
	}
}
