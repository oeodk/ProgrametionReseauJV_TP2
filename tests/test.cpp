#include <string>
#include <array>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "falcon.h"
#include "falcon_client.h"
#include "falcon_server.h"

#include "spdlog/spdlog.h"

using namespace std::chrono_literals;

TEST_CASE("Can Send", "[falcon ]")
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

    std::this_thread::sleep_for(1100ms);
    REQUIRE(client.IsConnected());
}

TEST_CASE("Disconnection from server death", "[falcon client]") {
    FalconClient client;
    {
        FalconServer server;
        server.Listen(5555);
        REQUIRE(server.IsListening());

        client.ConnectTo("127.0.0.1", 5555);
        std::this_thread::sleep_for(100ms);
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
        std::this_thread::sleep_for(100ms);
    }
    std::this_thread::sleep_for(1100ms);

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