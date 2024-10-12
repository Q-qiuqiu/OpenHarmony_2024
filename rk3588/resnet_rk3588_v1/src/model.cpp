#include <memory>
#include <iostream>
#include <cstring>
#include "model.h"

Context::Context(void *model_data, size_t model_size) : model_data(model_data), model_size(model_size) {
}

bool Context::init() {
    int ret;

    ret = rknn_init(&context, model_data, model_size, 0, nullptr);
    if (ret < 0) {
        std::cout << "rknn init failed, ret=" << ret << std::endl;
        return false;
    }

    return true;
}

Context::~Context() {
    if (context) {
        rknn_destroy(context);
    }
}

Model::Model(Context &context) : context(context) {
}

bool Model::init() {
    int ret;

    ret = rknn_query(context.get(), RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0) {
        std::cout << "rknn query in out num failed, ret=" << ret << std::endl;
        return false;
    }

    if (!get_inout_attrs(true, input_attrs, io_num.n_input) ||
        !get_inout_attrs(false, output_attrs, io_num.n_output)) {
        return false;
    }

    input_format = input_attrs[0].fmt;
    return true;
}

bool Model::get_inout_attrs(bool is_input, std::vector<rknn_tensor_attr> &attrs, size_t count) {
    attrs.resize(count);
    memset(attrs.data(), 0, sizeof(rknn_tensor_attr) * count);
    for (size_t i = 0; i < count; i++) {
        attrs[i].index = i;
        int ret = rknn_query(context.get(),
                             is_input ? RKNN_QUERY_INPUT_ATTR : RKNN_QUERY_OUTPUT_ATTR,
                             &attrs[i],
                             sizeof(attrs[i]));
        if (ret < 0) {
            std::cout << "rknn query " << (is_input ? "input" : "output") << i << " attr failed, ret=" << ret
                      << std::endl;
            return false;
        }
    }
    return true;
}

bool Model::execute(const std::vector<MemRange> &input) {
    if (input.size() != io_num.n_input) {
        std::cout << "input size mismatch, expect " << io_num.n_input << ", got " << input.size() << std::endl;
        return false;
    }
    std::vector<rknn_input> rknn_inputs = create_input_dataset(input);

    int ret;
    ret = rknn_inputs_set(context.get(), io_num.n_input, rknn_inputs.data());
    if (ret < 0) {
        std::cout << "rknn inputs set failed, ret=" << ret << std::endl;
        return false;
    }

    ret = rknn_run(context.get(), nullptr);
    if (ret < 0) {
        std::cout << "rknn run failed, ret=" << ret << std::endl;
        return false;
    }

    return true;
}

std::vector<rknn_input> Model::create_input_dataset(const std::vector<MemRange> &input) {
    std::vector<rknn_input> rknn_inputs;
    rknn_inputs.resize(io_num.n_input);
    memset(rknn_inputs.data(), 0, sizeof(rknn_input) * rknn_inputs.size());
    for (size_t i = 0; i < rknn_inputs.size(); i++) {
        rknn_inputs[i].index = i;
        rknn_inputs[i].buf = input[i].ptr;
        rknn_inputs[i].size = input[i].size;
        rknn_inputs[i].type = RKNN_TENSOR_UINT8;
        rknn_inputs[i].fmt = RKNN_TENSOR_NHWC;
    }
    return rknn_inputs;
}

OutputGuard Model::create_output_dataset() {
    std::vector<rknn_output> outputs;
    outputs.resize(io_num.n_output);
    memset(outputs.data(), 0, sizeof(rknn_output) * outputs.size());
    for (size_t i = 0; i < outputs.size(); i++) {
        outputs[i].index = i;
        outputs[i].want_float = 1;
    }

    int ret = rknn_outputs_get(context.get(), io_num.n_output, outputs.data(), nullptr);
    if (ret < 0) {
        std::cout << "rknn outputs get failed, ret=" << ret << std::endl;
        return {context, {}};
    }

    return {context, std::move(outputs)};
}
