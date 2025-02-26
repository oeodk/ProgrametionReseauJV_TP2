#include <string>
#include <array>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "falcon.h"
#include "falcon_client.h"
#include "falcon_server.h"

#include "spdlog/spdlog.h"

using namespace std::chrono_literals;

TEST_CASE("Can Send", "[falcon]")
{
    FalconServer server;
    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);

    int byte_sent = client.SendTo("127.0.0.1", 5555, "hello");
    REQUIRE(byte_sent > 0);
}

TEST_CASE("Can Receive", "[falcon]")
{
    FalconServer server;
    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);

    int byte_sent = client.SendTo("127.0.0.1", 5555, "hello");
    std::string from_ip;
    from_ip.resize(255);
    std::array<char, 65535> buffer;
    int byte_received = server.ReceiveFrom(from_ip, buffer);
    REQUIRE(byte_received > 0);
}

TEST_CASE( "Connection failed", "[falcon client]" ) {
    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    std::this_thread::sleep_for(1s);
    
    REQUIRE(client.IsConnected() == false);
}

TEST_CASE( "Can start", "[falcon server]" ) {
    FalconServer server;
    server.Listen(5555);
    REQUIRE(server.IsListening());
}

TEST_CASE( "Can Connect", "[falcon client]" ) {
    FalconServer server;
    server.Listen(5555);
    REQUIRE(server.IsListening());

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);

    std::this_thread::sleep_for(1500ms);
    REQUIRE(client.IsConnected());
}

TEST_CASE("Disconnection from server death", "[falcon client]") {
    FalconClient client;
    {
        FalconServer server;
        server.Listen(5555);
        REQUIRE(server.IsListening());

        client.ConnectTo("127.0.0.1", 5555);
        std::this_thread::sleep_for(500ms);
    }    
    std::this_thread::sleep_for(1500ms);

    REQUIRE(client.IsConnected() == false);
}

TEST_CASE("Client count updated", "[falcon server]") {
    FalconServer server;
    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    std::this_thread::sleep_for(100ms);

    REQUIRE(server.GetActiveClientCount() == 1);
}

TEST_CASE("Disconnection from client death", "[falcon server]") {
    FalconServer server;
    {
        server.Listen(5555);

        FalconClient client;
        client.ConnectTo("127.0.0.1", 5555);
        std::this_thread::sleep_for(500ms);
    }
    std::this_thread::sleep_for(1500ms);

    REQUIRE(server.GetActiveClientCount() == 0);
}

TEST_CASE("Connection kept thanks to Ping / Pong", "[falcon client]")
{
    FalconServer server;
    
    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    
    std::this_thread::sleep_for(5s);

    REQUIRE(client.IsConnected());
}

TEST_CASE("Connection kept thanks to Ping / Pong", "[falcon server]")
{
    FalconServer server;

    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    
    std::this_thread::sleep_for(5s);

    REQUIRE(server.GetActiveClientCount() == 1);
}

TEST_CASE("Can create a stream", "[falcon client]")
{
    FalconClient client;
    auto streamReliable = client.CreateStream(true);
    auto streamUnreliable = client.CreateStream(false);

    REQUIRE(streamReliable->GetStreamID() >= (1 << 31));
    REQUIRE(streamUnreliable->GetStreamID() < (1 << 31));
}

TEST_CASE("Can create a stream", "[falcon server]")
{
    FalconServer server;

    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    std::this_thread::sleep_for(500ms);

    auto streamReliable = server.CreateStream(0, true);
    auto streamUnreliable = server.CreateStream(0, false);

    REQUIRE(streamReliable->GetStreamID() >= (1 << 31));
    REQUIRE(streamUnreliable->GetStreamID() < (1 << 31));
}



TEST_CASE("Stream Acknowledge registers", "[falcon client]")
{
    FalconServer server;

    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    std::this_thread::sleep_for(500ms);

    auto stream = client.CreateStream(true);
    std::this_thread::sleep_for(200ms);
    client.SendData("Helo", stream->GetStreamID());
    REQUIRE(client.GetStreamsAck().size() == 1);

    std::this_thread::sleep_for(800ms);

    REQUIRE(client.GetStreamsAck().size() == 0);
}

TEST_CASE("Stream Acknowledge registers", "[falcon server]")
{
    FalconServer server;

    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    std::this_thread::sleep_for(500ms);

    auto stream = server.CreateStream(0, true);
    std::this_thread::sleep_for(200ms);
    server.SendData("Helo", client.GetId(), stream->GetStreamID());

    REQUIRE(server.GetStreamsAck().size() == 1);

    std::this_thread::sleep_for(800ms);

    REQUIRE(server.GetStreamsAck().size() == 0);
}

TEST_CASE("Can close stream", "[falcon]")
{
    FalconServer server;

    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    std::this_thread::sleep_for(500ms);
    auto stream = client.CreateStream(true);
    auto streamId = stream->GetStreamID();
    client.SendData("helo", streamId);
    std::this_thread::sleep_for(500ms);

    auto& serverStreams = server.GetStreams();
    REQUIRE(serverStreams.contains(client.GetId()));

    REQUIRE(serverStreams.at(client.GetId()).contains(streamId));
    auto serverStream = serverStreams.at(client.GetId()).at(streamId);
    
    server.CloseStream(*serverStream);
    std::this_thread::sleep_for(500ms);

    REQUIRE(client.GetStreams().contains(client.GetId()) == false);
}

TEST_CASE("Can receive data", "[falcon]")
{
    FalconServer server;

    server.Listen(5555);

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    std::this_thread::sleep_for(500ms);
    auto client_stream = client.CreateStream(true);

    client_stream->SetFlag(8, true);
    std::string msg("helo");

    auto streamId = client_stream->GetStreamID();
    client.SendData(msg, streamId);
    std::this_thread::sleep_for(500ms);
    auto server_stream = server.GetStreams().at(client.GetId()).at(streamId);

    REQUIRE(server_stream->getLastData() == msg);
    REQUIRE(server_stream->GetFlag(8) == true);
}