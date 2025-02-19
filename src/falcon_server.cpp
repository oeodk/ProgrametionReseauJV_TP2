#include "falcon_server.h"
#include "falcon_client.h"
#include "message_type.h"
#include <array>
#include "spdlog/spdlog.h"
using namespace std::chrono_literals;

inline static uint64_t usable_id = 0;
constexpr std::chrono::microseconds TIMEOUT = 1000ms;

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
	m_active_client_count++;
	if(handler != nullptr)
	{
		handler(m_new_client);
	}
}

void FalconServer::OnClientDisconnected(std::function<void(uint64_t)> handler)
{
	m_active_client_count--;
	if (handler != nullptr)
	{
		handler(m_last_disconnected_client);
	}
}

uint32_t FalconServer::GetNewStreamID(bool reliable, uint64_t client)
{
	uint32_t id = m_lastUsedStreamID[client]++;

	if (reliable)
		id = id & (1 << 31);

	return id;
}

void FalconServer::ThreadListen(FalconServer& server)
{
	std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> client_timeout;
	while (server.m_listen)
	{
		std::array<char, 65535> buffer;
		std::string other_ip;
		int recv_size = server.ReceiveFrom(other_ip, buffer);
		if (recv_size > 0)
		{
			uint64_t client_id = 0;
			if (MessageType(buffer[0]) != CONNECT)
			{
				memcpy(&client_id, &buffer[3], sizeof(client_id));
				if(client_timeout.contains(client_id))
				{
					client_timeout.at(client_id) = std::chrono::steady_clock::now();
				}
			}

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

				spdlog::debug("New client " + std::to_string(server.m_new_client));

				client_timeout.insert({ server.m_new_client, std::chrono::steady_clock::now() });

				server.m_clients.insert({ server.m_new_client , {ip, port} });

				std::string ack_message;
				const uint16_t msg_size = 12;
				ack_message.resize(msg_size);

				ack_message[0] = CONNECT_ACK;
				memcpy(&ack_message[1], &msg_size, sizeof(msg_size));
				memcpy(&ack_message[3], &server.m_new_client, sizeof(server.m_new_client));
				memcpy(&ack_message[11], &server.m_version, sizeof(server.m_version));

				server.SendTo(server.m_clients.at(server.m_new_client).ip, server.m_clients.at(server.m_new_client).port, ack_message);
				server.OnClientConnected(server.m_on_client_connect);
				
			}
				break;
			case DISCONNECT:
				server.m_last_disconnected_client = client_id;
				spdlog::debug("Disconnection from " + std::to_string(server.m_last_disconnected_client));
				server.OnClientDisconnected(server.m_on_client_disconnect);
				break;
			case PING:
			{
				spdlog::debug("Ping received from " + std::to_string(client_id));

				std::string pong_msg;
				std::chrono::system_clock::time_point time = std::chrono::system_clock::now();;
				const uint16_t msg_size = 13 + sizeof(time);
				pong_msg.resize(msg_size);

				uint16_t pong_id;
				memcpy(&pong_id, &buffer[11], sizeof(pong_id));

				pong_msg[0] = PONG;
				memcpy(&pong_msg[1], &msg_size, sizeof(msg_size));
				memcpy(&pong_msg[3], &client_id, sizeof(client_id));
				memcpy(&pong_msg[11], &pong_id, sizeof(pong_id));
				memcpy(&pong_msg[13], &time, sizeof(time));
				server.SendTo(server.m_clients.at(client_id).ip, server.m_clients.at(client_id).port, pong_msg);
			}
				break;
			case CREATE_STREAM:
			{
				uint32_t stream_id;
				memcpy(&stream_id, &buffer[7], sizeof(stream_id));

				server.m_streams[client_id].push_back(server.CreateStream(client_id, (stream_id & 1 << 31)));
			}
				break;
			case CLOSE_STREAM:
			{
				uint32_t stream_id;
				memcpy(&stream_id, &buffer[7], sizeof(stream_id));

				for (size_t i = 0; i < server.m_streams[client_id].size(); i++)
				{
					if (server.m_streams[client_id][i]->GetId() == stream_id)
					{
						server.m_streams[client_id].erase(server.m_streams[client_id].begin() + i);
						break;
					}
				}
				if (server.m_streams[client_id].size() == 0)
				{
					server.m_streams.erase(client_id);
				}
			}
				break;
			case DATA:
				//server.m_streams[client_id][stream_id].OnDataReceived(buffer);
				break;
			case DATA_ACK:
				break;
			}
			

		}
		std::vector<uint64_t> disconnected_client;
		disconnected_client.reserve(client_timeout.size());
		for (const auto& pair : client_timeout)
		{
			if (duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - pair.second) > TIMEOUT)
			{
				server.m_last_disconnected_client = pair.first;
				disconnected_client.push_back(pair.first);

				spdlog::debug("Client " + std::to_string(pair.first)+  " removed because of inactivity");

				server.OnClientDisconnected(server.m_on_client_disconnect);
			}
		}
		for (auto& id : disconnected_client)
		{
			client_timeout.erase(id);
		}
	}
}

std::unique_ptr<Stream> FalconServer::CreateStream(uint64_t client, bool reliable) {
	uint32_t stream_id = GetNewStreamID(reliable, client);
	std::unique_ptr<Stream> stream = std::make_unique<Stream>(
		stream_id,
		client,
		this->m_clients.at(client),
		this
	);

	uint16_t msg_size = 11;
	std::string message;
	message.resize(msg_size);

	message[0] = CREATE_STREAM;
	memcpy(&message[1], &msg_size, sizeof(msg_size));
	memcpy(&message[3], &client, sizeof(client));
	memcpy(&message[7], &stream_id, sizeof(stream_id));

	SendTo(m_clients.at(client).ip, m_clients.at(client).port, message);

	return stream;
}

void FalconServer::CloseStream(const Stream& stream) {
	uint64_t client_id = dynamic_cast<FalconClient*>(stream.getSocket())->GetId();
	uint32_t stream_id = stream.GetId();
	uint16_t msg_size = 11;
	std::string message;
	message.resize(msg_size);

	message[0] = CREATE_STREAM;
	memcpy(&message[1], &msg_size, sizeof(msg_size));
	memcpy(&message[3], &client_id, sizeof(client_id));
	memcpy(&message[7], &stream_id, sizeof(stream_id));

	SendTo(m_clients.at(client_id).ip, m_clients.at(client_id).port, message);
}