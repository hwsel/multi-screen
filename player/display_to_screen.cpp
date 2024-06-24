//
// Created by zichen on 5/28/24.
//
#include "display_to_screen.h"

#include <spdlog/spdlog.h>
#include <csignal>
#include <SDL.h>
#include <SDL_thread.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
}

#include "safe_queue.h"

extern std::atomic<bool> gExitSignal;
extern SafeQueue<AVFrame *> frameQueue;
extern int64_t OFFSET_TOLERANCE_US;

void display_to_screen()
{
    // init SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        spdlog::error("Could not init SDL {}", SDL_GetError());
        std::raise(SIGINT);
    }

    // TODO: should wait for size
    // TODO: resize, key input
    // SDL_Window *pSDLWindow = SDL_CreateWindow("Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920 / 2,
    //                                           1080 / 2, SDL_WINDOW_OPENGL);

    // SDL_Renderer *pSDLRenderer = SDL_CreateRenderer(pSDLWindow, -1, 0);
    // SDL_Texture *pSDLTexture = SDL_CreateTexture(pSDLRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, 1920,
    //                                              1080);
    // Query the display mode to get the current screen resolution
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);

    SDL_Window *pSDLWindow = SDL_CreateWindow("Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, displayMode.w, displayMode.h, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);

    SDL_Renderer *pSDLRenderer = SDL_CreateRenderer(pSDLWindow, -1, 0);
    SDL_Texture *pSDLTexture = SDL_CreateTexture(pSDLRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, 1920, 1080);

    AVFrame *pAVFrame;
    AVFrame *pAVFrameHold;
    size_t queueSize;

    while (true)
    {
        if (gExitSignal)
        {
            spdlog::debug("exiting display...");
            break;
        }


        if (frameQueue.dequeue(pAVFrame))
        {
            if (pAVFrame)
            {
                spdlog::debug(
                        "Get frame (type={}, size={} bytes, format={}) pts {} key_frame {} [DTS {}] buffer {}",
                        av_get_picture_type_char(pAVFrame->pict_type),
                        pAVFrame->pkt_size,
                        pAVFrame->format,
                        pAVFrame->pts,
                        pAVFrame->key_frame,
                        pAVFrame->coded_picture_number,
                        queueSize
                );

                int64_t ts = 0;
                // get ts
                if (pAVFrame->nb_side_data > 0)
                {
                    ts = strtoll(reinterpret_cast<const char *>(pAVFrame->side_data[0]->data + 16 + 11), nullptr, 10);
                    int64_t currentTs = av_gettime();
                    spdlog::debug("TS: {}", currentTs - ts);
                    if (currentTs - ts > OFFSET_TOLERANCE_US)
                        continue;

                    if (currentTs - ts < 0)
                    {
                        if (queueSize > 0)
                            continue;
                        spdlog::debug("future frame, sleep for {} us", ts - currentTs);
                        std::this_thread::sleep_for(std::chrono::microseconds{ts - currentTs});
                    } else if (currentTs - ts < OFFSET_TOLERANCE_US)
                    { ;
                    } else
                    {
                        continue;
                    }
//                    spdlog::info("TELEMETRY: TS {} PTS {} @ {} / {}", ts, pAVFrame->pts, pAVFrame->time_base.num,
//                                 pAVFrame->time_base.den);

                }


//            SDL_UpdateTexture(pSDLTexture, nullptr, pAVFrame->data[0], pAVFrame->linesize[0]);
                // TODO: shomething is wrong here, the pixfmt is YUV444P, but it doesn't work, emmm.......
                SDL_UpdateYUVTexture(pSDLTexture, nullptr,
                                     pAVFrame->data[0], pAVFrame->linesize[0],
                                     pAVFrame->data[1], pAVFrame->linesize[1],
                                     pAVFrame->data[2], pAVFrame->linesize[2]);


                SDL_RenderClear(pSDLRenderer);
                SDL_RenderCopy(pSDLRenderer, pSDLTexture, nullptr, nullptr);
                SDL_RenderPresent(pSDLRenderer);

//                spdlog::info("TELE {} {}", ts, av_gettime());
//                spdlog::info("TELEMETRY: TS {} PTS {} @ {} / {}", ts, pAVFrame->pts, pAVFrame->time_base.num, pAVFrame->time_base.den);

                spdlog::info("TELEMETRY: SEI_TS {} DISPLAY_TS {} PTS {} FPS {} / {} QUEUE_LEN {}", ts, av_gettime(), pAVFrame->pts,
                             pAVFrame->time_base.num, pAVFrame->time_base.den, frameQueue.queue_size());

                if (pAVFrameHold) av_frame_free(&pAVFrameHold);
//                av_frame_ref(pAVFrameHold, pAVFrame);
                pAVFrameHold = av_frame_clone(pAVFrame);
            }
        }

        av_frame_free(&pAVFrame);
    }

    SDL_Quit();
}