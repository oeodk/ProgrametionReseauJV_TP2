#include <iostream>

#include <falcon.h>
#include <falcon_client.h>

#include "spdlog/spdlog.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Hello World!");

    FalconClient client;
    client.ConnectTo("127.0.0.1", 5555);
    while (true);
    return EXIT_SUCCESS;
}
