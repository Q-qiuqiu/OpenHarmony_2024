#include <iostream>
#include <sstream>
#include <mutex>
#include <future>
#include <queue>
#include <opencv2/opencv.hpp>
#include "httplib.h"
#include "json.hpp"
#include "video_transcoder.h"

using namespace httplib;
using json = nlohmann::json;

static std::string make_result(const std::string &status, const json &result) {
    json j;
    j["status"] = status;
    if (!result.empty()) {
        j["reason"] = result;
    }
    return j.dump();
}

static std::string make_success(const json &result) {
    return make_result("success", result);
}

static std::string make_failed(const json &result) {
    return make_result("failed", result);
}

static void fill_response(httplib::Response &res, const std::string &content) {
    res.set_content(content, "application/json");
}

int main() {
    FfmpegGlobalInit global_init;
    if (!global_init.init()) {
        std::cerr << "global init failed" << std::endl;
        return 1;
    }

    VideoTranscoderManager manager;
    Server server;

    server.Post("/start", [&](const Request &req, Response &res) {
        std::string url = req.get_param_value("url");
        if (url.empty()) {
            fill_response(res, make_failed("need url"));
            return;
        }
        std::string bit_rate_str = req.get_param_value("bit_rate");
        if (bit_rate_str.empty()) {
            fill_response(res, make_failed("need bit_rate"));
            return;
        }
        int64_t bit_rate = std::stoll(bit_rate_str);
        if (bit_rate <= 0) {
            fill_response(res, make_failed("bit_rate must be positive"));
            return;
        }

        if (auto result = manager.add_url(url, bit_rate); result != VideoTranscoderManager::Result::SUCCESS) {
            fill_response(res, make_failed(manager.parse_result(result)));
            return;
        }
        std::cout << "start transcoding " << url << std::endl;
        fill_response(res, make_success({}));
    });

    server.Post("/stop", [&](const Request &req, Response &res) {
        std::string url = req.get_param_value("url");
        if (url.empty()) {
            fill_response(res, make_failed("need url"));
            return;
        }
        if (auto result = manager.remove_url(url); result != VideoTranscoderManager::Result::SUCCESS) {
            fill_response(res, make_failed(manager.parse_result(result)));
            return;
        }
        std::cout << "stop transcoding " << url << std::endl;
        fill_response(res, make_success({}));
    });

    std::cout << "start http server" << std::endl;
    if (!server.listen("0.0.0.0", 8080)) {
        std::cerr << "http server start failed" << std::endl;
        return 1;
    }
    return 0;
}
