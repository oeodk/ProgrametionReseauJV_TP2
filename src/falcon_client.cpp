#include "falcon_client.h"
#include "message_type.h"
#include <array>
#include "spdlog/spdlog.h"

using namespace std::chrono_literals;

constexpr std::chrono::microseconds TIMEOUT = 1000ms;
constexpr std::chrono::microseconds ACK_CHECK = 500ms;

FalconClient::~FalconClient()
{
	m_listen = false;
	if(m_listener.joinable())
	{
		m_listener.join();
	}
}

void FalconClient::ConnectTo(const std::string& ip, uint16_t port)
{
	Falcon::CreateClient(ip);
	m_listen = true;
	m_listener = std::thread(ThreadListen, std::ref(*this));
	server.ip = ip;
	server.port = port;

	std::string connection_message;
	const uint16_t msg_size = 4;
	connection_message.resize(msg_size);

	connection_message[0] = CONNECT;
	memcpy(&connection_message[1], &msg_size, sizeof(msg_size));
	memcpy(&connection_message[3], &m_version, sizeof(m_version));

	SendTo(server.ip, server.port, connection_message);
}

void FalconClient::OnConnectionEvent(std::function<void(bool, uint64_t)> handler)
{
	if(handler != nullptr)
	{
		handler(m_connected, m_id);
	}
}

void FalconClient::OnDisconnect(std::function<void()> handler)
{
	m_connected = false;
	if (handler != nullptr)
	{
		handler();
	}
}



void FalconClient::SendData(std::span<const char> data, uint32_t stream_id)
{ 
	if(m_streams.contains(stream_id))
	{
		m_streams.at(stream_id)->SendData(data);
		if (stream_id & 1 << 31)
		{
			m_streams_ack.insert({ stream_id, data });
		}
	}
}

void FalconClient::ThreadListen(FalconClient& client)
{
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point timeout_timer = start;
	std::chrono::steady_clock::time_point ack_check = std::chrono::steady_clock::now();

	uint16_t ping_id = 0;

	while(client.m_listen)
	{
		std::array<char, 65535> buffer;
		std::string other_ip;
		int recv_size = client.ReceiveFrom(other_ip, buffer);


		if(client.m_connected)
		{
			std::string ping_msg;
			std::chrono::system_clock::time_point time = std::chrono::system_clock::now();;
			const uint16_t msg_size = 13 + sizeof(time);
			ping_msg.resize(msg_size);

			ping_msg[0] = PING;
			memcpy(&ping_msg[1], &msg_size, sizeof(msg_size));
			memcpy(&ping_msg[3], &client.m_id, sizeof(client.m_id));
			memcpy(&ping_msg[11], &ping_id, sizeof(ping_id));
			memcpy(&ping_msg[13], &time, sizeof(time));
			client.SendTo(client.server.ip, client.server.port, ping_msg);

			spdlog::debug("Ping sent");
		}

		ping_id++;
		if (recv_size > 0)
		{
			timeout_timer = std::chrono::steady_clock::now();
			switch (MessageType(buffer[0]))
			{
			case CONNECT_ACK:
			{
				client.m_connected = true;
				memcpy(&client.m_id, &buffer[3], sizeof(client.m_id));
				client.OnConnectionEvent(client.m_on_connect);
				spdlog::debug("Connection ACK received");
			}
			break;
			case DISCONNECT:
				client.m_listen = false;
				client.OnDisconnect(client.m_on_disconnect);
				spdlog::debug("Disconnect received");
				break;
			case PONG:
			{
				spdlog::debug("Pong received");
			}
			break;
			case CLOSE_STREAM:
			{
				uint32_t stream_id;
				memcpy(&stream_id, &buffer[11], sizeof(stream_id));
				for (size_t i = 0; i < client.m_local_streams.size(); i++)
				{
					if (client.m_local_streams[i]->GetStreamID() == stream_id)
					{
						client.m_local_streams.erase(client.m_local_streams.begin() + i);
						break;
					}
				}

				client.m_streams.erase(stream_id);

			}
			break;
			case DATA:
			{
				uint32_t stream_id;
				memcpy(&stream_id, &buffer[11], sizeof(stream_id));

				if (!client.m_streams.contains(stream_id))
				{
					client.m_local_streams.push_back(client.MakeStream(stream_id, stream_id & RELIABLE_STREAM_BIT));
				}
				client.m_streams.at(stream_id)->OnDataReceived(buffer);
			}
				break;
			case DATA_ACK:
				{
					uint32_t stream_id;
					memcpy(&stream_id, &buffer[11], sizeof(stream_id));
					if (client.m_streams_ack.contains(stream_id))
					{
						client.m_streams_ack.erase(stream_id);
					}
				}
				break;
			}
		}
		else
		{
			if (!client.m_connected)
			{
				if (duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start) > TIMEOUT)
				{
					client.m_listen = false;
					client.OnConnectionEvent(client.m_on_connect);
					spdlog::debug("Connection failed");
				}
			}
			else
			{
				auto a = duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timeout_timer);
				if (duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timeout_timer) > TIMEOUT)
				{
					client.m_listen = false;
					client.OnDisconnect(client.m_on_disconnect);
					spdlog::debug("Disconnection due to inactivity");
				}

			}
		}
		if (duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - ack_check) > ACK_CHECK)
		{
			for (auto& pair : client.m_streams_ack)
			{			
				client.m_streams.at(pair.first)->SendData(pair.second);
			}
			ack_check = std::chrono::steady_clock::now();
		}
	}
}
std::unique_ptr<Stream> FalconClient::MakeStream(uint32_t stream_id, bool reliable)
{
	std::unique_ptr<Stream> stream = std::make_unique<Stream>(
		stream_id,
		m_id,
		server,
		this
	);

	m_streams.insert({ stream_id, stream.get()});
	return stream;
}

std::unique_ptr<Stream> FalconClient::CreateStream(bool reliable) {
	return MakeStream(GetNewStreamID(reliable), reliable);
}

uint32_t FalconClient::GetNewStreamID(bool reliable)
{
	uint32_t id = m_lastUsedStreamID;
	m_lastUsedStreamID++;

	if (reliable)
		id = id | RELIABLE_STREAM_BIT;

	return id;
}