#include "falcon_client.h"
#include "message_type.h"
#include <array>
#include "spdlog/spdlog.h"

using namespace std::chrono_literals;

constexpr std::chrono::microseconds TIMEOUT = 1000ms;

FalconClient::~FalconClient()
{
	m_listen = false;
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

void FalconClient::ThreadListen(FalconClient& client)
{
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point timeout_timer = start;

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
			case CREATE_STREAM:
				//client.m_streams.push_back(CreateStream())
				break;
			case CLOSE_STREAM:
				//server.m_streams[stream_id].CloseStream()
				break;
			case DATA:
				//server.m_streams[stream_id].OnDataReceived(buffer);
				break;
			case DATA_ACK:
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
	}
}

std::unique_ptr<Stream> FalconClient::CreateStream(bool reliable) {
	std::unique_ptr<Stream> stream = std::make_unique<Stream>(
		GetNewStreamID(reliable),
		m_id,
		server,
		this
	);

	// Envoyer trame

	return stream;
}