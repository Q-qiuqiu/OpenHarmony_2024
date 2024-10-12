#ifndef VIDEO_DECODER_VIDEO_DECODER_H
#define VIDEO_DECODER_VIDEO_DECODER_H

#include <string>
#include <functional>
#include <future>
#include <utility>
#include <map>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class FfmpegGlobalInit {
public:
    bool init();

    ~FfmpegGlobalInit();
};

class VideoTranscoder {
public:
    VideoTranscoder(std::string url, int64_t bit_rate): url(std::move(url)), bit_rate(bit_rate) {}

    ~VideoTranscoder();

    bool init();

    bool run();

    bool stop();

private:
    std::string url{};
    int64_t bit_rate{};
    bool init_success{};
    std::atomic_bool want_stop{};
    std::future<bool> run_result{};
    AVFormatContext *decode_format_ctx{};
    int video_index{};
    AVCodecContext *decode_codec_ctx{};
    AVCodecContext *encode_codec_ctx{};
    AVPacket *av_decode_packet{};
    AVPacket *av_encode_packet{};
    AVFrame *av_frame{};

    bool run_impl();

    AVCodec *find_hardware_decoder(AVCodecID id);

    AVCodec *find_hardware_encoder(AVCodecID id);
};

class VideoTranscoderManager {
public:
    enum Result {
        SUCCESS,
        FAILED,
        NOT_FOUND,
        DUPLICATED,
    };

    Result add_url(const std::string &url, int64_t bit_rate);

    Result remove_url(const std::string &url);

    std::string parse_result(Result result);

private:
    std::map<std::string, std::unique_ptr<VideoTranscoder>> items;
    std::mutex mutex;
};


#endif //VIDEO_DECODER_VIDEO_DECODER_H
