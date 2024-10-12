#include <iostream>
#include <chrono>
#include "video_transcoder.h"


bool FfmpegGlobalInit::init() {
    // init network
    if (avformat_network_init() < 0) {
        return false;
    }
    return true;
}

FfmpegGlobalInit::~FfmpegGlobalInit() {
    avformat_network_deinit();
}

bool VideoTranscoder::init() {
    // open url
    if (avformat_open_input(&decode_format_ctx, url.c_str(), nullptr, nullptr) < 0) {
        return false;
    }

    // get stream info
    if (avformat_find_stream_info(decode_format_ctx, nullptr) < 0) {
        return false;
    }

    // extract video info
    AVStream *av_stream{};
    AVCodecParameters *av_codec_params{};
    for (int i = 0; i < decode_format_ctx->nb_streams; i++) {
        if (decode_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            av_stream = decode_format_ctx->streams[i];
            av_codec_params = av_stream->codecpar;
        }
    }
    if (!av_codec_params) {
        return false;
    }

    // open decoder
    AVCodec *decode_codec = find_hardware_decoder(av_codec_params->codec_id);
    if (!decode_codec) {
        return false;
    }

    decode_codec_ctx = avcodec_alloc_context3(decode_codec);
    if (!decode_codec_ctx) {
        return false;
    }

    if (avcodec_parameters_to_context(decode_codec_ctx, av_codec_params) < 0) {
        return false;
    }

    if (avcodec_open2(decode_codec_ctx, decode_codec, nullptr) < 0) {
        return false;
    }

    // open encoder
    AVCodec *encode_codec = find_hardware_encoder(av_codec_params->codec_id);
    if (!encode_codec) {
        return false;
    }

    encode_codec_ctx = avcodec_alloc_context3(encode_codec);
    if (!encode_codec_ctx) {
        return false;
    }

    encode_codec_ctx->width = decode_codec_ctx->width;
    encode_codec_ctx->height = decode_codec_ctx->height;
    encode_codec_ctx->pix_fmt = decode_codec_ctx->pix_fmt;
    encode_codec_ctx->time_base = av_stream->time_base;
    encode_codec_ctx->framerate = av_stream->r_frame_rate;
    encode_codec_ctx->bit_rate = bit_rate;

    if (avcodec_open2(encode_codec_ctx, encode_codec, nullptr) < 0) {
        return false;
    }

    // allocate packet and frame
    av_decode_packet = av_packet_alloc();
    if (!av_decode_packet) {
        return false;
    }
    av_encode_packet = av_packet_alloc();
    if (!av_encode_packet) {
        return false;
    }
    av_frame = av_frame_alloc();
    if (!av_frame) {
        return false;
    }

    init_success = true;
    return true;
}

AVCodec *VideoTranscoder::find_hardware_decoder(AVCodecID id) {
    AVCodec *codec = const_cast<AVCodec *>(avcodec_find_decoder(id));
    if (!codec) {
        return nullptr;
    }
    // try to find rkmpp decoder
    std::string new_name = std::string(codec->name) + "_rkmpp";
    const AVCodec *hardware_codec = avcodec_find_decoder_by_name(new_name.c_str());
    if (hardware_codec) {
        codec = const_cast<AVCodec *>(hardware_codec);
    }

    // print decoder name
    std::cout << "decoder name: " << codec->name << std::endl;
    return codec;
}

AVCodec *VideoTranscoder::find_hardware_encoder(AVCodecID id) {
    const AVCodec *codec = nullptr;
    switch (id) {
        case::AV_CODEC_ID_H264:
            codec = avcodec_find_encoder_by_name("h264_rkmpp");
            if (!codec) {
                codec = avcodec_find_encoder(id);
            }
            break;
        default:
            break;
    }
    if (codec) {
        std::cout << "encoder name: " << codec->name << std::endl;
    }
    return const_cast<AVCodec *>(codec);
}

static std::string av_err2string(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return {errbuf};
}

class FpsCounter {
public:
    FpsCounter(std::string name) : name(std::move(name)), count(0),
                                   last_time(std::chrono::high_resolution_clock::now()) {}

    void tick() {
        count++;
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
        if (duration >= 1000) {
            std::cout << "fps: " << count << std::endl;
            count = 0;
            last_time = now;
        }
    }

private:
    std::string name;
    int count;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
};

class DecodeDurationCounter {
public:
    DecodeDurationCounter(std::string name) : name(std::move(name)), count(0), count_duration(0),
                                              last_time(std::chrono::high_resolution_clock::now()) {}

    void begin() {
        begin_time = std::chrono::high_resolution_clock::now();
    }

    void end() {
        end_time = std::chrono::high_resolution_clock::now();
        count_duration += std::chrono::duration_cast<std::chrono::milliseconds>(end_time - begin_time).count();

        count++;
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - last_time).count();
        if (duration >= 1000) {
            if (count == 0) {
                std::cout << name << " decode duration: UNKNOWN" << std::endl;
            } else {
                std::cout << name << " decode duration: " << ((double) count_duration / count) << "ms" << std::endl;
            }
            count_duration = 0;
            count = 0;
            last_time = end_time;
        }
    }

    double get_ms() {
        return (double) std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count() / 1000.0;
    }

private:
    std::string name;
    int count;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
    long long count_duration;
};

bool VideoTranscoder::run() {
    if (!init_success) {
        return false;
    }
    run_result = std::async(std::launch::async, &VideoTranscoder::run_impl, this);
    return true;
}

bool VideoTranscoder::stop() {
    want_stop = true;
    return run_result.get();
}

static bool need_retry(int ret) {
    return ret == AVERROR(EAGAIN) || ret == AVERROR_EXTERNAL;
}

bool VideoTranscoder::run_impl() {
    int ret = 0;
    bool error = false;
    FpsCounter fps_counter(url);

    // read frame
    while (!want_stop && !error) {
        ret = av_read_frame(decode_format_ctx, av_decode_packet);
        if (ret >= 0) {
            if (av_decode_packet->stream_index == video_index) {
                ret = avcodec_send_packet(decode_codec_ctx, av_decode_packet);
                if (ret >= 0) {
                    ret = avcodec_receive_frame(decode_codec_ctx, av_frame);
                    if (ret >= 0) {
                        av_frame->pts = av_frame->best_effort_timestamp;
                        // decode frame
                        ret = avcodec_send_frame(encode_codec_ctx, av_frame);
                        if (ret >= 0) {
                            ret = avcodec_receive_packet(encode_codec_ctx, av_encode_packet);
                            if (ret >= 0) {
                                // do something with packet
                                fps_counter.tick();
                                av_packet_unref(av_encode_packet);
                            } else if (need_retry(ret)) {
                                std::cout << "avcodec_receive_packet retry: " << av_err2string(ret) << std::endl;
                            } else {
                                std::cout << "avcodec_receive_packet failed: " << av_err2string(ret) << std::endl;
                                error = true;
                            }
                        } else if (need_retry(ret)) {
                            std::cout << "avcodec_send_frame retry: " << av_err2string(ret) << std::endl;
                        } else {
                            std::cout << "avcodec_send_frame failed: " << av_err2string(ret) << std::endl;
                            error = true;
                        }

                    } else if (need_retry(ret)) {
                        std::cout << "avcodec_receive_frame retry: " << av_err2string(ret) << std::endl;
                    } else {
                        std::cout << "avcodec_receive_frame failed: " << av_err2string(ret) << std::endl;
                        error = true;
                    }

                } else if (need_retry(ret)) {
                    avcodec_flush_buffers(decode_codec_ctx);
                    std::cout << "avcodec_send_packet retry: " << av_err2string(ret) << std::endl;
                } else {
                    std::cout << "avcodec_send_packet failed: " << av_err2string(ret) << std::endl;
                    error = true;
                }
            }

            av_packet_unref(av_decode_packet);
        } else if (need_retry(ret)) {
            std::cout << "av_read_frame retry: " << av_err2string(ret) << std::endl;
        } else {
            std::cout << "av_read_frame failed: " << av_err2string(ret) << std::endl;
            error = true;
        }
    }
    return !error;
}

VideoTranscoder::~VideoTranscoder() {
    if (av_frame) {
        av_frame_free(&av_frame);
    }
    if (av_encode_packet) {
        av_packet_free(&av_encode_packet);
    }
    if (av_decode_packet) {
        av_packet_free(&av_decode_packet);
    }
    if (encode_codec_ctx) {
        avcodec_free_context(&encode_codec_ctx);
    }
    if (decode_codec_ctx) {
        avcodec_free_context(&decode_codec_ctx);
    }
    if (decode_format_ctx) {
        avformat_close_input(&decode_format_ctx);
    }
}

VideoTranscoderManager::Result VideoTranscoderManager::add_url(const std::string &url, int64_t bit_rate) {
    std::lock_guard lock(mutex);

    if (items.find(url) != items.end()) {
        return Result::DUPLICATED;
    }

    auto item = std::make_unique<VideoTranscoder>(url, bit_rate);
    if (!item->init() || !item->run()) {
        return Result::FAILED;
    }
    items[url] = std::move(item);
    return Result::SUCCESS;
}

VideoTranscoderManager::Result VideoTranscoderManager::remove_url(const std::string &url) {
    std::lock_guard lock(mutex);

    auto it = items.find(url);
    if (it == items.end()) {
        return Result::NOT_FOUND;
    }

    bool result = it->second->stop();
    items.erase(it);
    return result ? Result::SUCCESS : Result::FAILED;
}

std::string VideoTranscoderManager::parse_result(VideoTranscoderManager::Result result) {
    switch (result) {
        case Result::SUCCESS:
            return "success";
        case Result::FAILED:
            return "task failed";
        case Result::DUPLICATED:
            return "url duplicated";
        case Result::NOT_FOUND:
            return "url not found";
        default:
            return "unknown";
    }
}
