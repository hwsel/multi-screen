//
// Created by zichen on 5/28/24.
//
#include "receive_from_server.h"
#include "safe_queue.h"


#include <string>
#include <spdlog/spdlog.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
}

extern std::atomic<bool> gExitSignal;
extern SafeQueue<AVFrame *> frameQueue;

void print_stream_info_general(AVFormatContext *pAVFormatContext, int stream_idx)
{
    spdlog::debug(
            "AVStream->time_base before open coded {}/{}",
            pAVFormatContext->streams[stream_idx]->time_base.num,
            pAVFormatContext->streams[stream_idx]->time_base.den
    );
    spdlog::debug(
            "AVStream->r_frame_rate before open coded {}/{}",
            pAVFormatContext->streams[stream_idx]->r_frame_rate.num,
            pAVFormatContext->streams[stream_idx]->r_frame_rate.den
    );
    spdlog::debug(
            "AVStream->start_time {}",
            pAVFormatContext->streams[stream_idx]->start_time
    );
    spdlog::debug(
            "AVStream->duration {}",
            pAVFormatContext->streams[stream_idx]->duration
    );
}

void print_stream_info_video(AVFormatContext *pAVFormatContext, int stream_idx)
{
    print_stream_info_general(pAVFormatContext, stream_idx);

    AVCodecParameters *pAVCodecParameters = pAVFormatContext->streams[stream_idx]->codecpar;
    const AVCodec *pAVCodec = avcodec_find_decoder(pAVCodecParameters->codec_id);
    if (!pAVCodec)
    {
        spdlog::error("Unsupported codec");
        return;
    }

    if (pAVCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        spdlog::debug(
                "Video Codec: resolution {} x {}",
                pAVCodecParameters->width,
                pAVCodecParameters->height
        );
    }
}

static int decode_packet(AVPacket *pAVPacket, AVCodecContext *pAVCodecContext, AVFrame *pAVFrame)
{
    int response = avcodec_send_packet(pAVCodecContext, pAVPacket);

    if (response < 0)
    {
        spdlog::error("Error while sending a packet to the decoder");
        return response;
    }

    while (response >= 0)
    {
        response = avcodec_receive_frame(pAVCodecContext, pAVFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            break;
        } else if (response < 0)
        {
            spdlog::error("Error while receiving a frame from the decoder");
            return response;
        }

        if (response >= 0)
        {
            // TODO: send AVFrame to queue here
            AVFrame* pAVFrameSentToQueue = av_frame_alloc();
            av_frame_ref(pAVFrameSentToQueue, pAVFrame);
            frameQueue.enqueue(pAVFrameSentToQueue);


//            has_frame = true;
//            if (currentPlayer == gCurrentPlayer)
//            {
//                AVFrame *pAVFrameSentToQueue = av_frame_clone(pAVFrame);
//                {
//                    const std::lock_guard<std::mutex> lock(gMainFrameQueueLock);
//                    gMainFrameQueue.push(pAVFrameSentToQueue);
//                    gMainFrameQueueCondLock.notify_all();
//
//                }
//            }

            spdlog::debug(
                    "Frame {} (type={}, size={} bytes, format={}) pts {} key_frame {} [DTS {}]",
                    pAVCodecContext->frame_number,
                    av_get_picture_type_char(pAVFrame->pict_type),
                    pAVFrame->pkt_size,
                    pAVFrame->format,
                    pAVFrame->pts,
                    pAVFrame->key_frame,
                    pAVFrame->coded_picture_number
            );
        }
    }

    return 0;
}

int receive_from_server(std::string url)
{
    int result;
    AVFormatContext *pAVFormatContext = avformat_alloc_context();

    // init ffmpeg framework
    if (!pAVFormatContext)
    {
        spdlog::error("Could not allocate memory for Format Context.");
        return -1;
    }

    // setup options for the framework
    AVDictionary *pAVFormatContextOptions = nullptr;
    av_dict_set_int(&pAVFormatContextOptions, "probesize", 5000000 * 60, 0);
    av_dict_set_int(&pAVFormatContextOptions, "analyzeduration", 5000000 * 60, 0);

    // open the stream
    spdlog::debug("open the stream");
    result = avformat_open_input(&pAVFormatContext, url.c_str(), nullptr, &pAVFormatContextOptions);
    if (result != 0)
    {
        spdlog::error("Could not open the file. Error code: {}", result);
        return -1;
    }


    // locate the video stream
    spdlog::debug(
            "format {}, duration {} us, bit_rate {}",
            pAVFormatContext->iformat->name,
            pAVFormatContext->duration,
            pAVFormatContext->bit_rate
    );

    result = avformat_find_stream_info(pAVFormatContext, nullptr);
    if (result < 0)
    {
        spdlog::error("Could not get the stream info");
        return -1;
    }

    int video_stream_index = av_find_best_stream(pAVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    if (video_stream_index < 0)
    {
        spdlog::error("Stream does not contain a video stream");
        return -1;
    }

    print_stream_info_video(pAVFormatContext, video_stream_index);

    // prepare codec
    AVCodecParameters *pAVCodecParameters = pAVFormatContext->streams[video_stream_index]->codecpar;
    const AVCodec *pAVCodec = avcodec_find_decoder(pAVCodecParameters->codec_id);

    if (!pAVCodec)
    {
        spdlog::error("Unsupported codec");
        return -1;
    }

    // prepare codec context
    AVCodecContext *pAVCodecContext = avcodec_alloc_context3(pAVCodec);
    if (!pAVCodecContext)
    {
        spdlog::error("failed to allocated memory for AVCodecContext");
        return -1;
    }

    result = avcodec_parameters_to_context(pAVCodecContext, pAVCodecParameters);
    if (result < 0)
    {
        spdlog::error("failed to copy codec params to codec context");
        return -1;
    }

    // open the codec
    result = avcodec_open2(pAVCodecContext, pAVCodec, nullptr);
    if (result < 0)
    {
        spdlog::error("failed to open codec through avcodec_open2");
        return -1;
    }

    // create frame slot
    AVFrame *pAVFrame = av_frame_alloc();
    if (!pAVFrame)
    {
        spdlog::error("failed to allocate memory for AVFrame");
        return -1;
    }

    // create packet slot
    AVPacket *pAVPacket = av_packet_alloc();
    if (!pAVPacket)
    {
        spdlog::error("failed to allocate memory for AVPacket");
        return -1;
    }

    while (av_read_frame(pAVFormatContext, pAVPacket) >= 0)
    {
        // TODO exit signal
        if (gExitSignal)
        {
            spdlog::debug("exiting player...");
            break;
        }

        // process the video stream packet
        if (pAVPacket->stream_index == video_stream_index)
        {
            spdlog::debug("AVPacket->pts {}", pAVPacket->pts);

            spdlog::debug("Queue length {}", frameQueue.queue_size());

            result = decode_packet(pAVPacket, pAVCodecContext, pAVFrame);
            if (result < 0)
            {
                break;
            }
        }

    }

    // cleanup
    avformat_close_input(&pAVFormatContext);
    av_packet_free(&pAVPacket);
    av_frame_free(&pAVFrame);
    avcodec_free_context(&pAVCodecContext);

    spdlog::info("player exit");


    return 0;
}