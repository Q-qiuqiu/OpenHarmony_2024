#ifndef RESNET_INFER_SRC_MODEL_H
#define RESNET_INFER_SRC_MODEL_H

#include <string>
#include <vector>
#include <optional>
#include <rknn_api.h>

class Context {
public:
    Context(void *model_data, size_t model_size);

    bool init();

    ~Context();

    rknn_context get() {
        return context;
    }

private:
    void *model_data;
    size_t model_size;
    rknn_context context{};
};

struct MemRange {
    void *ptr;
    size_t size;
};

class OutputGuard {
public:
    // disable copy constructor and copy assignment
    OutputGuard(const OutputGuard &) = delete;
    OutputGuard &operator=(const OutputGuard &) = delete;

    OutputGuard(Context &context, std::vector<rknn_output> outputs) : context(context), outputs(std::move(outputs)) {
    }

    ~OutputGuard() {
        if (outputs.empty()) {
            return;
        }

        int ret;
        ret = rknn_outputs_release(context.get(), outputs.size(), outputs.data());
        if (ret < 0) {
            std::cout << "rknn outputs release failed, ret=" << ret << std::endl;
        }
    }

    std::vector<rknn_output> &get() {
        return outputs;
    }

private:
    Context &context;
    std::vector<rknn_output> outputs;
};

class Model {
public:
    explicit Model(Context &context);

    bool init();

    bool execute(const std::vector<MemRange> &input);

    OutputGuard create_output_dataset();

protected:
    Context &context;
    rknn_input_output_num io_num{};
    std::vector<rknn_tensor_attr> input_attrs;
    std::vector<rknn_tensor_attr> output_attrs;
    rknn_tensor_format input_format{};

    bool get_inout_attrs(bool is_input, std::vector<rknn_tensor_attr> &attrs, size_t size);

    std::vector<rknn_input> create_input_dataset(const std::vector<MemRange> &input);
};

#endif //RESNET_INFER_SRC_MODEL_H
