#ifndef STREAM_H
#define STREAM_H

#include <memory>
#include <span>

class Stream {
private:
    uint32_t id;

public:
    Stream() = default;
    ~Stream();
    Stream(const Stream&) = default;
    Stream& operator=(const Stream&) = default;
    Stream(Stream&&) = default;
    Stream& operator=(Stream&&) = default;

    std::unique_ptr<Stream> CreateStream(uint64_t client, bool reliable);
    std::unique_ptr<Stream> CreateStream(bool reliable);

    void CloseStream(const Stream& stream);

    void SendData(std::span<const char> Data);
    void OnDataReceived(std::span<const char> Data);
};


#endif //STREAM_H
