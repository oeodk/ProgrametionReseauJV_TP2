#include "Stream.h"

Stream::~Stream() {

}

std::unique_ptr<Stream> Stream::CreateStream(uint64_t client, bool reliable) {
	return std::unique_ptr<Stream>();
}

std::unique_ptr<Stream> Stream::CreateStream(bool reliable) {
	return std::unique_ptr<Stream>();
}

void Stream::CloseStream(const Stream& stream) {

}

void Stream::SendData(std::span<const char> Data) {

}

void Stream::OnDataReceived(std::span<const char> Data) {

}