#include <iostream>

#include <falcon.h>
#include <falcon_server.h>
#include "spdlog/spdlog.h"

int main()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Hello World!");
    FalconServer server;
    server.Listen(5555);
    while (true);
    return EXIT_SUCCESS;
}
