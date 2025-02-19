#include "Stream.h"
#include "message_type.h"
#include <string>

constexpr int HEADER_SIZE = 184;

Stream::Stream(uint32_t _stream_id, uint64_t _client_uuid, IpPortPair _target, Falcon* _socket):
	stream_id(_stream_id), client_uuid(_client_uuid), target(_target), socket(_socket)
{}

Stream::~Stream() {

}

uint16_t Stream::GetNewMessageID() {
	uint16_t id = msg_id;
	msg_id++;

	return id;
}

void Stream::SetFlag(int flag_id, bool value) {
	if (flag_id < 0 || flag_id >= 16) return;

	if (value)
		flags = flags | (1 << flag_id);
	else
		flags = flags & ~(1 << flag_id);
}

bool Stream::GetFlag(int flag_id)
{
	if (flag_id < 0 || flag_id >= 16) return false;
	uint16_t mask;
	mask = 1 << flag_id;
	return flags & mask;
}

void Stream::SendData(std::span<const char> data) {
	int max_packet_size = 65536 - HEADER_SIZE;
	const uint8_t part_total = data.size() / max_packet_size + 1;

	for (uint8_t part_id = 0; part_id < part_total; part_id++) {
		SendDataPart(part_id, part_total, data.subspan(part_id * max_packet_size, max_packet_size));
	}
}

void Stream::SendDataPart(uint8_t part_id, uint8_t part_total, std::span<const char> data) {
	std::string message;
	
	const uint16_t data_size = data.size();
	const uint16_t message_size = data_size + HEADER_SIZE;
	message.resize(message_size);
	uint16_t message_id = GetNewMessageID();


	int current_pos = 0;

	message[current_pos] = DATA;
	current_pos += sizeof(DATA);

	memcpy(&message[current_pos], &message_size, sizeof(message_size));
	current_pos += sizeof(message_size);

	memcpy(&message[current_pos], &client_uuid, sizeof(client_uuid));
	current_pos += sizeof(client_uuid);

	memcpy(&message[current_pos], &stream_id, sizeof(stream_id));
	current_pos += sizeof(stream_id);

	memcpy(&message[current_pos], &data_size, sizeof(data_size));
	m_msg_size_index = current_pos;

	current_pos += sizeof(data_size);

	memcpy(&message[current_pos], &flags, sizeof(flags));
	m_flag_index = current_pos;
	current_pos += sizeof(flags);

	memcpy(&message[current_pos], &part_id, sizeof(part_id));
	current_pos += sizeof(part_id);

	memcpy(&message[current_pos], &part_total, sizeof(part_total));
	current_pos += sizeof(part_total);

	memcpy(&message[current_pos], &message_id, sizeof(message_id));
	current_pos += sizeof(message_id);

	memcpy(&message[current_pos], &data, sizeof(data));

	socket->SendTo(target.ip, target.port, message);
}



void Stream::OnDataReceived(std::span<const char> data) {
	
	uint16_t data_size;
	memcpy(&data_size, &data[m_msg_size_index], sizeof(uint16_t));
	memcpy(&flags, &data[m_flag_index], sizeof(uint16_t));
	
	std::string message;

	const uint16_t message_size = data_size + 7;
	message.resize(message_size);
	int current_pos = 0;

	message[current_pos] = DATA_ACK;
	current_pos += sizeof(DATA);

	memcpy(&message[current_pos], &message_size, sizeof(message_size));
	current_pos += sizeof(message_size);

	memcpy(&message[current_pos], &client_uuid, sizeof(client_uuid));
	current_pos += sizeof(client_uuid);

	memcpy(&message[current_pos], &data, sizeof(data));

	socket->SendTo(target.ip, target.port, message);

	memcpy(&data, &data, sizeof(data_size));

}