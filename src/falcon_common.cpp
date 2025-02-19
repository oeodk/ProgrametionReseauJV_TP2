#include "falcon.h"

int Falcon::SendTo(const std::string &to, uint16_t port, const std::span<const char> message)
{
    return SendToInternal(to, port, message);
}

int Falcon::ReceiveFrom(std::string& from, const std::span<char, 65535> message)
{
    return ReceiveFromInternal(from, message);
}

uint32_t Falcon::GetNewStreamID(bool reliable) {
    uint32_t id = lastUsedStreamID;
    lastUsedStreamID++;

    if (reliable)
        id = id & (1 << 31);

    return id;
}