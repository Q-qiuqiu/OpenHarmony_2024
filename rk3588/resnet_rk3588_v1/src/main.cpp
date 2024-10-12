#include <iostream>
#include <fstream>
#include <memory>
#include <opencv2/opencv.hpp>
#include <sched.h>
#include "model.h"
#include "resnet.h"
#include "httplib.h"
#include "json.hpp"

using namespace nlohmann;

static std::string make_result(const std::string &status, const json &result) {
    json j;
    j["status"] = status;
    j["result"] = result;
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

static json infer_result_to_json(const InferResult &infer_result) {
    json j;
    j["index"] = infer_result.index;
    j["score"] = infer_result.score;
    return j;
}

static std::string read_file(const std::string &filename) {
    std::ifstream f(filename);
    return {(std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()};
}

class ThreadContext {
public:
    ThreadContext(void *model_data, size_t model_size) :
            context(model_data, model_size),
            model(context) {
        if (!context.init()) {
            std::cout << "context init failed" << std::endl;
            return;
        }
        if (!model.init()) {
            std::cout << "model init failed" << std::endl;
            return;
        }
        valid = true;
    }

    bool is_valid() {
        return valid;
    }

    Resnet &get_model() {
        return model;
    }

private:
    bool valid{};
    Context context;
    Resnet model;
};

int main() {
    std::string model_file = read_file("model/resnet.rknn");

    auto get_thread_context = [&model_file]() -> ThreadContext & {
        thread_local ThreadContext context(model_file.data(), model_file.size());
        return context;
    };

    // start http server
    httplib::Server server;

    server.Post("/predict", [&](const httplib::Request &req, httplib::Response &res) {
        ThreadContext &context = get_thread_context();
        if (!context.is_valid()) {
            fill_response(res, make_failed("init model failed"));
            return;
        }

        httplib::MultipartFormData http_image = req.get_file_value("image");
        // decode image
        std::vector<uint8_t> image_data(http_image.content.begin(), http_image.content.end());
        cv::Mat image = cv::imdecode(image_data, cv::IMREAD_COLOR);
        if (image.empty()) {
            fill_response(res, make_failed("decode image failed"));
            return;
        }
        InferResult infer_result = context.get_model().infer(image);
        fill_response(res, make_success(infer_result_to_json(infer_result)));
    });

    server.Post("/predict_raw", [&](const httplib::Request &req, httplib::Response &res) {
        ThreadContext &context = get_thread_context();
        if (!context.is_valid()) {
            fill_response(res, make_failed("init model failed"));
            return;
        }

        httplib::MultipartFormData http_width = req.get_file_value("width");
        httplib::MultipartFormData http_height = req.get_file_value("height");
        httplib::MultipartFormData http_image = req.get_file_value("image");
        if (http_width.content.empty() || http_height.content.empty() || http_image.content.empty()) {
            fill_response(res, make_failed("invalid params"));
            return;
        }

        size_t width = std::stoul(http_width.content);
        size_t height = std::stoul(http_height.content);

        cv::Mat image(height, width, CV_8UC3, http_image.content.data());

        InferResult infer_result = context.get_model().infer(image);
        fill_response(res, make_success(infer_result_to_json(infer_result)));
    });

    std::cout << "start http server" << std::endl;
    if (!server.listen("0.0.0.0", 8080)) {
        std::cout << "http server start failed" << std::endl;
        return 1;
    }

    return 0;
}
