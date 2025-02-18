#include "Stream.h"

std::unique_ptr<Stream> Stream::CreateStream(uint64_t client, bool reliable) {

}

std::unique_ptr<Stream> Stream::CreateStream(bool reliable) {

}

void Stream::CloseStream(const Stream& stream) {

}

void Stream::SendData(std::span<const char> Data) {

}

void Stream::OnDataReceived(std::span<const char> Data) {

}