#include "napi/native_api.h"
#include "common.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include "json.hpp"
#include "httplib.h"
using namespace std;
using namespace cv;
using json = nlohmann::json;
static const char *TAG = "[qiuqiu]";
const std::vector<std::string> labelsList = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic_light",
    "fire_hydrant", "stop_sign", "parking_meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow", "elephant",
    "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard",
    "sports_ball", "kite", "baseball_bat", "baseball_glove", "skateboard", "surfboard", "tennis_racket", "bottle",
    "wine_glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli",
    "carrot", "hot_dog", "pizza", "donut", "cake", "chair", "couch", "potted_plant", "bed", "dining_table", "toilet",
    "tv", "laptop", "mouse", "remote", "keyboard", "cell_phone", "microwave", "oven", "toaster", "sink", "refrigerator",
    "book", "clock", "vase", "scissors", "teddy_bear", "hair_drier", "toothbrush"};

void ParseJsonAndConvert(napi_env env, const std::string& str, 
                         std::vector<cv::Rect>& rectangles, 
                         std::vector<int>& labels, 
                         std::vector<double>& probabilities) {
    json root = json::parse(str);

    const auto& resultArray = root["result"];
    for (const auto& item : resultArray) {
        // 提取坐标
        double x0 = item[0].get<double>();
        double y0 = item[1].get<double>();
        double x1 = item[2].get<double>();
        double y1 = item[3].get<double>();
        rectangles.emplace_back(cv::Rect(cv::Point(x0, y0), cv::Point(x1, y1)));

        // 提取概率
        double probability = item[4].get<double>();
        probabilities.push_back(probability);

        // 提取标签
        int label = item[5].get<int>();
        labels.push_back(label);  // 假设标签是整数，转换为字符串
    }
}
// 使用OpenCV绘制矩形框
void DrawRectangles(cv::Mat& img, const std::vector<cv::Rect>& rectangles, const std::vector<int>& labels, const std::vector<double>& probabilities) {
    for (size_t i = 0; i < rectangles.size(); ++i) {
        const auto& rect = rectangles[i];

        int labelindex = labels[i];
        double probability = probabilities[i];

        // 获取标签文字
        std::string label = labelsList[labelindex];
        // 绘制矩形框
        cv::rectangle(img, rect, cv::Scalar(0, 0, 255), 8);

        // 绘制标签和概率
        std::string text = label + " (" + std::to_string(probability) + ")";
        cv::putText(img, text, cv::Point(rect.x, rect.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
    }
}

napi_value HttpGet(napi_env env, napi_callback_info info) {
    auto start4 = std::chrono::high_resolution_clock::now(); // 记录开始时间
    napi_value result;
    napi_status status;
    size_t argc = 2;
    napi_value argv[2];
    status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (status != napi_ok) {
        return nullptr;
    }

    // 获取传入的第一个URL字符串
    size_t str_size1;
    size_t str_size_read1;
    status = napi_get_value_string_utf8(env, argv[0], nullptr, 0, &str_size1);
    if (status != napi_ok) {
        return nullptr;
    }

    std::vector<char> url1(str_size1 + 1);
    status = napi_get_value_string_utf8(env, argv[0], url1.data(), str_size1 + 1, &str_size_read1);
    if (status != napi_ok) {
        return nullptr;
    }

    // 获取传入的第二个URL字符串
    size_t str_size2;
    size_t str_size_read2;
    status = napi_get_value_string_utf8(env, argv[1], nullptr, 0, &str_size2);
    if (status != napi_ok) {
        return nullptr;
    }
    std::vector<char> url2(str_size2 + 1);
    status = napi_get_value_string_utf8(env, argv[1], url2.data(), str_size2 + 1, &str_size_read2);
    if (status != napi_ok) {
        return nullptr;
    }

    // 使用 httplib 发送HTTP GET请求
    std::string url_str1(url1.data());
    httplib::Client cli(url_str1.c_str());
    std::string url_str2(url2.data());
    auto res = cli.Get(("/video?url=" + url_str2).c_str());
    if (!res) {
         OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "res is null");
         return nullptr;
    }
    if (res->status != 200) {
        OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "res error, status:  %{public}d, content_length:  %{public}zu", res->status, res->content_length_);
        return nullptr;
    }

    // 直接使用 HTTP 响应的数据创建 ArrayBuffer
    napi_value arraybuffer;
    void* buffer_data;
    status = napi_create_arraybuffer(env, res->body.size(), &buffer_data, &arraybuffer);
    if (status != napi_ok) {
        return nullptr;
    }

    memcpy(buffer_data, res->body.data(), res->body.size());

    // 将ArrayBuffer解析成字节数组
    uint8_t* byteData = static_cast<uint8_t*>(buffer_data);
    size_t length = res->body.size();

    // 找到叹号的位置
       size_t exclamationIndex = length;
       for (size_t i = 0; i < length; ++i) {
           if (byteData[i] == '!') {
               exclamationIndex = i;
               break;
           }
       }
       napi_create_object(env, &result);
       napi_value jsStatus;
       if (exclamationIndex == length) {
               OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server No exclamation mark found in data");
               status = napi_create_string_utf8(env, "failed", NAPI_AUTO_LENGTH, &jsStatus);
                       napi_set_named_property(env, result, "status", jsStatus);
               return result;
           }else{
               status = napi_create_string_utf8(env, "success", NAPI_AUTO_LENGTH, &jsStatus);
                       napi_set_named_property(env, result, "status", jsStatus);
        }

       if (exclamationIndex == length) {
           OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server No exclamation mark found in data");
           return result;
       }

       //解析叹号之前的数据为字符串
       std::string str(reinterpret_cast<char*>(byteData), exclamationIndex);
       json root;
       try {
           root = json::parse(str);
       } catch (json::parse_error& e) {
           OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server Failed to parse JSON");
           return nullptr;
       }

       // 创建返回的对象
       napi_create_object(env, &result);

       // 解析 JSON 数据并转换为绘制矩形框所需的格式
       std::vector<cv::Rect> rectangles;
       std::vector<int> labels;
       std::vector<double> probabilities;
       ParseJsonAndConvert(env, str, rectangles, labels, probabilities);

       // 叹号之后的数据处理
       uint8_t* remainingData = byteData + exclamationIndex + 1;
       int width = 640;  // 替换为实际宽度
       int height = 360; // 替换为实际高度

       // 将字节数组转换为 OpenCV Mat 对象
       Mat srcImage(height, width, CV_8UC3, remainingData);

        // 绘制矩形框
       DrawRectangles(srcImage, rectangles, labels, probabilities);


       // 将 BGR 图像转换为 BGRA 图像
       Mat srcImageBGRA;
       cvtColor(srcImage, srcImageBGRA, COLOR_BGR2BGRA);

       // 将图像数据转换为 ArrayBuffer 格式
       size_t bufferSize = srcImageBGRA.total() * srcImageBGRA.elemSize();
       napi_value outputBuffer;
       void* bufferData;
       // 创建 ArrayBuffer
       status = napi_create_arraybuffer(env, bufferSize, &bufferData, &outputBuffer);
       if (status != napi_ok) {
           std::cerr << "Failed to create ArrayBuffer" << std::endl;
           return result;
           }
       // 将 BGRA 数据拷贝到 ArrayBuffer
       memcpy(bufferData, srcImageBGRA.data, bufferSize);

       napi_set_named_property(env, result, "byteBuffer", outputBuffer);

       return result;  // 返回创建的对象
}

napi_value HttpGet_png(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_status status;
    size_t argc = 2;
    napi_value argv[2];
    status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (status != napi_ok) {
        return nullptr;
    }

    // 获取传入的第一个URL字符串
    size_t str_size1;
    size_t str_size_read1;
    status = napi_get_value_string_utf8(env, argv[0], nullptr, 0, &str_size1);
    if (status != napi_ok) {
        return nullptr;
    }

    std::vector<char> url1(str_size1 + 1);
    status = napi_get_value_string_utf8(env, argv[0], url1.data(), str_size1 + 1, &str_size_read1);
    if (status != napi_ok) {
        return nullptr;
    }

    // 获取传入的第二个URL字符串
    size_t str_size2;
    size_t str_size_read2;
    status = napi_get_value_string_utf8(env, argv[1], nullptr, 0, &str_size2);
    if (status != napi_ok) {
        return nullptr;
    }
    std::vector<char> url2(str_size2 + 1);
    status = napi_get_value_string_utf8(env, argv[1], url2.data(), str_size2 + 1, &str_size_read2);
    if (status != napi_ok) {
        return nullptr;
    }

    // 使用 httplib 发送HTTP GET请求
    std::string url_str1(url1.data());
    httplib::Client cli(url_str1.c_str());
    std::string url_str2(url2.data());
    auto res = cli.Get(("/video?url=" + url_str2).c_str());
    if (!res) {
         OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "res is null");
         return nullptr;
    }
    if (res->status != 200) {
        OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "res error, status:  %{public}d, content_length:  %{public}zu", res->status, res->content_length_);
        return nullptr;
    }

    // 直接使用 HTTP 响应的数据创建 ArrayBuffer
    napi_value arraybuffer;
    void* buffer_data;
    status = napi_create_arraybuffer(env, res->body.size(), &buffer_data, &arraybuffer);
    if (status != napi_ok) {
        return nullptr;
    }
    memcpy(buffer_data, res->body.data(), res->body.size());
    // 将ArrayBuffer解析成字节数组
    uint8_t* byteData = static_cast<uint8_t*>(buffer_data);
    size_t length = res->body.size();

    // 找到叹号的位置
       size_t exclamationIndex = length;
       for (size_t i = 0; i < length; ++i) {
           if (byteData[i] == '!') {
               exclamationIndex = i;
               break;
           }
       }
       napi_create_object(env, &result);
       napi_value jsStatus;
       if (exclamationIndex == length) {
               OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server No exclamation mark found in data");
               status = napi_create_string_utf8(env, "failed", NAPI_AUTO_LENGTH, &jsStatus);
                       napi_set_named_property(env, result, "status", jsStatus);
               return result;
           }else{
               status = napi_create_string_utf8(env, "success", NAPI_AUTO_LENGTH, &jsStatus);
                       napi_set_named_property(env, result, "status", jsStatus);
        }

       if (exclamationIndex == length) {
           OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server No exclamation mark found in data");
           return result;
       }

       //解析叹号之前的数据为字符串
       std::string str(reinterpret_cast<char*>(byteData), exclamationIndex);
       json root;
       try {
           root = json::parse(str);
       } catch (json::parse_error& e) {
           OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server Failed to parse JSON");
           return nullptr;
       }

//       // 创建返回的对象
//       napi_create_object(env, &result);
//    
//        // 提取并设置 weight 和 height
//        napi_value weight, height;
//        napi_create_double(env, root["weight"].get<double>(), &weight);
//        napi_create_double(env, root["height"].get<double>(), &height);
//        
//        napi_set_named_property(env, result, "weight", weight);
//        napi_set_named_property(env, result, "height", height);
    
    
    
       // 解析 JSON 数据并转换为绘制矩形框所需的格式
       std::vector<cv::Rect> rectangles;
       std::vector<int> labels;
       std::vector<double> probabilities;
       ParseJsonAndConvert(env, str, rectangles, labels, probabilities);

   // 叹号之后的数据处理
   uint8_t* remainingData = byteData + exclamationIndex + 1;
   std::vector<uint8_t> pngData(remainingData, remainingData + (length - exclamationIndex - 1)); // 获取PNG数据

   // 使用imdecode解码PNG数据
   cv::Mat srcImage = cv::imdecode(pngData, cv::IMREAD_COLOR);
   if (srcImage.empty()) {
       OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "Failed to decode PNG image");
       return result;
   }
    
    // 绘制矩形框
    DrawRectangles(srcImage, rectangles, labels, probabilities);
    // 将 BGR 图像转换为 BGRA 图像
    Mat srcImageBGRA;
    cvtColor(srcImage, srcImageBGRA, COLOR_BGR2BGRA);
           
    // 将图像数据转换为 ArrayBuffer 格式
    size_t bufferSize = srcImageBGRA.total() * srcImageBGRA.elemSize();
    napi_value outputBuffer;
    void* bufferData;
    // 创建 ArrayBuffer
    status = napi_create_arraybuffer(env, bufferSize, &bufferData, &outputBuffer);
    if (status != napi_ok) {
        std::cerr << "Failed to create ArrayBuffer" << std::endl;
        return result;
        }
    // 将 BGRA 数据拷贝到 ArrayBuffer
    memcpy(bufferData, srcImageBGRA.data, bufferSize);
           
    napi_set_named_property(env, result, "byteBuffer", outputBuffer);
    return result;  // 返回创建的对象
}

napi_value Img2Gray(napi_env env, napi_callback_info info) {
    OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server Img2Gray Begin");
    napi_value result = NapiGetNull(env);
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    // 获取字节数组
    napi_typedarray_type type;
    size_t length;
    void* data;
    napi_value arraybuffer;
    size_t byteOffset;
    napi_status status = napi_get_typedarray_info(env, argv[0], &type, &length, &data, &arraybuffer, &byteOffset);

    if (status != napi_ok || type != napi_uint8_array) {
        OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server Expected a Uint8Array");
        return result;
    }
    uint8_t* byteData = static_cast<uint8_t*>(data);
    // 找到叹号的位置
    size_t exclamationIndex = length;
    for (size_t i = 0; i < length; ++i) {
        if (byteData[i] == '!') {
            exclamationIndex = i;
            break;
        }
    }

    if (exclamationIndex == length) {
        OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server No exclamation mark found in data");
        return result;
    }

    //解析叹号之前的数据为字符串
    std::string str(reinterpret_cast<char*>(byteData), exclamationIndex);
    json root;
    try {
        root = json::parse(str);
    } catch (json::parse_error& e) {
        OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server Failed to parse JSON");
        return nullptr;
    }

    // 创建返回的对象
    napi_create_object(env, &result);

    // 解析 JSON 数据并转换为绘制矩形框所需的格式
    std::vector<cv::Rect> rectangles;
    std::vector<int> labels;
    std::vector<double> probabilities;
    ParseJsonAndConvert(env, str, rectangles, labels, probabilities);

    // 叹号之后的数据处理
    uint8_t* remainingData = byteData + exclamationIndex + 1;
    int width = 640;  // 替换为实际宽度
    int height = 360; // 替换为实际高度

    // 将字节数组转换为 OpenCV Mat 对象
    Mat srcImage(height, width, CV_8UC3, remainingData);

     // 绘制矩形框
    DrawRectangles(srcImage, rectangles, labels, probabilities);


    // 将 BGR 图像转换为 BGRA 图像
    Mat srcImageBGRA;
    cvtColor(srcImage, srcImageBGRA, COLOR_BGR2BGRA);

    // 将图像数据转换为 ArrayBuffer 格式
    size_t bufferSize = srcImageBGRA.total() * srcImageBGRA.elemSize();
    napi_value outputBuffer;
    void* bufferData;
    // 创建 ArrayBuffer
    status = napi_create_arraybuffer(env, bufferSize, &bufferData, &outputBuffer);
    if (status != napi_ok) {
        std::cerr << "Failed to create ArrayBuffer" << std::endl;
        return result;
        }
    // 将 BGRA 数据拷贝到 ArrayBuffer
    memcpy(bufferData, srcImageBGRA.data, bufferSize);

    napi_set_named_property(env, result, "byteBuffer", outputBuffer);
    return result;  // 返回创建的对象
   }

napi_value Img2Gray_png(napi_env env, napi_callback_info info) {
    OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server Img2Gray_png Begin");
    napi_value result = NapiGetNull(env);
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    // 获取字节数组
    napi_typedarray_type type;
    size_t length;
    void* data;
    napi_value arraybuffer;
    size_t byteOffset;
    napi_status status = napi_get_typedarray_info(env, argv[0], &type, &length, &data, &arraybuffer, &byteOffset);

    if (status != napi_ok || type != napi_uint8_array) {
        OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server Expected a Uint8Array");
        return result;
    }
    uint8_t* byteData = static_cast<uint8_t*>(data);
    // 找到叹号的位置
    size_t exclamationIndex = length;
    for (size_t i = 0; i < length; ++i) {
        if (byteData[i] == '!') {
            exclamationIndex = i;
            break;
        }
    }

    if (exclamationIndex == length) {
        OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server No exclamation mark found in data");
        return result;
    }

    //解析叹号之前的数据为字符串
    std::string str(reinterpret_cast<char*>(byteData), exclamationIndex);
    json root;
    try {
        root = json::parse(str);
    } catch (json::parse_error& e) {
        OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "server Failed to parse JSON");
        return nullptr;
    }

    // 创建返回的对象
    napi_create_object(env, &result);

    // 解析 JSON 数据并转换为绘制矩形框所需的格式
    std::vector<cv::Rect> rectangles;
    std::vector<int> labels;
    std::vector<double> probabilities;
    ParseJsonAndConvert(env, str, rectangles, labels, probabilities);

    // 叹号之后的数据处理
    uint8_t* remainingData = byteData + exclamationIndex + 1;
    std::vector<uint8_t> pngData(remainingData, remainingData + (length - exclamationIndex - 1)); // 获取PNG数据

    // 使用imdecode解码PNG数据
    cv::Mat srcImage = cv::imdecode(pngData, cv::IMREAD_COLOR);
    if (srcImage.empty()) {
        OH_LOG_Print(LOG_APP, LOG_INFO, GLOBAL_RESMGR, TAG, "Failed to decode PNG image");
        return result;
    }
    
    // 绘制矩形框
    DrawRectangles(srcImage, rectangles, labels, probabilities);


    // 将 BGR 图像转换为 BGRA 图像
       Mat srcImageBGRA;
       cvtColor(srcImage, srcImageBGRA, COLOR_BGR2BGRA);
    
       // 将图像数据转换为 ArrayBuffer 格式
       size_t bufferSize = srcImageBGRA.total() * srcImageBGRA.elemSize();
       napi_value outputBuffer;
       void* bufferData;
       // 创建 ArrayBuffer
       status = napi_create_arraybuffer(env, bufferSize, &bufferData, &outputBuffer);
       if (status != napi_ok) {
           std::cerr << "Failed to create ArrayBuffer" << std::endl;
           return result;
           }
       // 将 BGRA 数据拷贝到 ArrayBuffer
       memcpy(bufferData, srcImageBGRA.data, bufferSize);
    
       napi_set_named_property(env, result, "byteBuffer", outputBuffer);
       return result;  // 返回创建的对象
       }
EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
         {"HttpGet", nullptr, HttpGet, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"HttpGet_png", nullptr, HttpGet_png, nullptr, nullptr, nullptr, napi_default, nullptr},
    {"Img2Gray", nullptr, Img2Gray, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"Img2Gray_png", nullptr, Img2Gray_png, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&demoModule);
}
