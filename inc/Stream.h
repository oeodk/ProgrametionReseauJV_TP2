#ifndef STREAM_H
#define STREAM_H

#include <memory>
#include <span>
#include "falcon.h"

class Stream {
private:
    uint32_t stream_id;
    uint16_t msg_id;
    uint64_t client_uuid;

    uint16_t flags;

    IpPortPair target;
    Falcon* socket;

public:
    Stream(uint32_t stream_id, uint64_t client_uuid, IpPortPair target, Falcon* socket);
    ~Stream();
    Stream(const Stream&) = default;
    Stream& operator=(const Stream&) = default;
    Stream(Stream&&) = default;
    Stream& operator=(Stream&&) = default;

    uint16_t GetNewMessageID();
    void SetFlag(int flag_id, bool value);
    bool GetFlag(int flag_id);

    uint32_t GetStreamID() const { return stream_id; }
    uint64_t GetClientUUID() const { return client_uuid; }
    IpPortPair GetTargetIpPortPair() const { return target; }
    Falcon* GetSocket() const { return socket; }

    void SendData(std::span<const char> data);
    void OnDataReceived(std::span<const char> data);
protected:
    void SendDataPart(uint8_t part_id, uint8_t part_total, std::span<const char> data);
};


#endif //STREAM_H
