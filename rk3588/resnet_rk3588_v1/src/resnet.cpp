#include <algorithm>
#include "resnet.h"

InferResult Resnet::infer(const cv::Mat &image) {
    cv::Mat new_image;
    cv::resize(image, new_image, cv::Size(wanted_width, wanted_height));
    cv::cvtColor(new_image, new_image, cv::COLOR_BGR2RGB);

    MemRange input_range{new_image.data, wanted_width * wanted_height * 3};
    std::vector<MemRange> input_ranges = {input_range};

    if (!execute(input_ranges)) {
        return {};
    }

    return parse_output();
}

void softmax(std::vector<float> &vec) {
    float max = *std::max_element(vec.begin(), vec.end());
    float sum = 0;
    for (float &v : vec) {
        v = std::exp(v - max);
        sum += v;
    }
    for (float &v : vec) {
        v /= sum;
    }
}

InferResult Resnet::parse_output() {
    if (io_num.n_output != 1) {
        return {};
    }

    OutputGuard outputs = create_output_dataset();

    rknn_output &output = outputs.get().at(0);
    float *output_data = reinterpret_cast<float *>(output.buf);

    std::vector<float> scores(output.size / sizeof(float));
    std::copy(output_data, output_data + scores.size(), scores.begin());

    softmax(scores);

    size_t index = std::distance(scores.begin(), std::max_element(scores.begin(), scores.end()));
    float score = scores[index];

    return {index, score};
}
