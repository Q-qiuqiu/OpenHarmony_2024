#ifndef RESNET_INFER_SRC_RESNET_H
#define RESNET_INFER_SRC_RESNET_H

#include <memory>
#include <vector>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include "model.h"

struct InferResult {
    size_t index;
    float score;
};

class Resnet : public Model {
public:
    using Model::Model;

    InferResult infer(const cv::Mat &image);

private:
    size_t wanted_height = 224;
    size_t wanted_width = 224;

    InferResult parse_output();
};

#endif //RESNET_INFER_SRC_RESNET_H
