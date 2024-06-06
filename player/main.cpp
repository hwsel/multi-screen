//
// Created by zichen on 5/28/24.
//

#include <string>

#include <spdlog/spdlog.h>
#include <csignal>
#include <thread>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/log.h>
}

#include "receive_from_server.h"
#include "safe_queue.h"
#include "display_to_screen.h"

int64_t OFFSET_TOLERANCE_US = 1000000;

// TODO: ugly global variables, should use some elegant implementations
std::atomic<bool> gExitSignal = false;

std::thread tPlayer;
std::thread tDisplay;

SafeQueue<AVFrame *> frameQueue;

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        gExitSignal = true;
    }
}

int main(int argc, char **argv)
{
    // reg signals
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    spdlog::set_level(spdlog::level::info);
    av_log_set_level(AV_LOG_FATAL);

    if (argc < 2)
    {
        spdlog::error("missing server url.");
        return -1;
    }

    const std::string server_url = argv[1];
    spdlog::info("will connect to {}", server_url);

    // ==================================================================
    spdlog::debug("initializing player");

    tPlayer = std::thread{receive_from_server, server_url};
    tDisplay = std::thread{display_to_screen};

    while (true)
    {
        // TODO exit signal
        if (gExitSignal)
        {
            break;
        }
    }

    tPlayer.join();
    tDisplay.join();

    frameQueue.stop_queue();

    return 0;
}