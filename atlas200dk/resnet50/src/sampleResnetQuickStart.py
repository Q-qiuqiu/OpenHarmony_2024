# -*- coding: utf-8 -*-
import os
import cv2
import numpy as np
import acl
from label import label
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import cgi
# 常量定义
NPY_FLOAT32 = 11
ACL_MEMCPY_HOST_TO_HOST = 0
ACL_MEMCPY_HOST_TO_DEVICE = 1
ACL_MEMCPY_DEVICE_TO_HOST = 2
ACL_MEMCPY_DEVICE_TO_DEVICE = 3
ACL_MEM_MALLOC_HUGE_FIRST = 0
ACL_DEVICE, ACL_HOST = 0, 1
ACL_SUCCESS = 0
min_chn = np.array([123.675, 116.28, 103.53], dtype=np.float32)
var_reci_chn = np.array([0.0171247538316637, 0.0175070028011204, 0.0174291938997821], dtype=np.float32)

class Sample_resnet_quick_start(object):
    # 初始化方法
    def __init__(self, device_id, model_path, model_width, model_height):
        self.device_id = device_id
        self.context = None
        self.stream = None
        self.model_width = model_width
        self.model_height = model_height
        self.model_id = None
        self.model_path = model_path
        self.model_desc = None
        self.input_dataset = None
        self.output_dataset = None
        self.input_buffer = None
        self.output_buffer = None
        self.input_buffer_size = None
        self.image_bytes = None
        self.image_name = None
        self.dir = None
        self.image = None
        self.runMode_ = acl.rt.get_run_mode()

    # 初始化资源
    def init_resource(self):
        ret = acl.init()
        if ret != ACL_SUCCESS:
            print('acl init failed, errorCode is', ret)

        ret = acl.rt.set_device(self.device_id)
        if ret != ACL_SUCCESS:
            print('set device failed, errorCode is', ret)
            
        self.context, ret = acl.rt.create_context(self.device_id)
        if ret != ACL_SUCCESS:
            print('create context failed, errorCode is', ret)

        self.stream, ret = acl.rt.create_stream()
        if ret != ACL_SUCCESS:
            print('create stream failed, errorCode is', ret)

        self.model_id, ret = acl.mdl.load_from_file(self.model_path)
        if ret != ACL_SUCCESS:
            print('load model failed, errorCode is', ret)

        self.model_desc = acl.mdl.create_desc()
        ret = acl.mdl.get_desc(self.model_desc, self.model_id)
        if ret != ACL_SUCCESS:
            print('get desc failed, errorCode is', ret)

        self.input_dataset = acl.mdl.create_dataset()
        input_index = 0
        self.input_buffer_size = acl.mdl.get_input_size_by_index(self.model_desc, input_index)
        self.input_buffer, ret = acl.rt.malloc(self.input_buffer_size, ACL_MEM_MALLOC_HUGE_FIRST)
        input_data = acl.create_data_buffer(self.input_buffer, self.input_buffer_size)
        self.input_dataset, ret = acl.mdl.add_dataset_buffer(self.input_dataset, input_data)
        if ret != ACL_SUCCESS:
            print('acl.mdl.add_dataset_buffer failed, errorCode is', ret)

        self.output_dataset = acl.mdl.create_dataset()
        output_index = 0
        output_buffer_size = acl.mdl.get_output_size_by_index(self.model_desc, output_index)
        self.output_buffer, ret = acl.rt.malloc(output_buffer_size, ACL_MEM_MALLOC_HUGE_FIRST)
        output_data = acl.create_data_buffer(self.output_buffer, output_buffer_size)
        self.output_dataset, ret = acl.mdl.add_dataset_buffer(self.output_dataset, output_data)
        if ret != ACL_SUCCESS:
            print('acl.mdl.add_dataset_buffer failed, errorCode is', ret)

    # 处理输入
    def process_input(self, image_data):
        # 从字节流读取图像
        image = cv2.imdecode(np.frombuffer(image_data, np.uint8), cv2.IMREAD_COLOR)
        self.image = image
        resize_image = cv2.resize(image, (self.model_width, self.model_height))
        image_rgb = cv2.cvtColor(resize_image, cv2.COLOR_BGR2RGB)
        image_rgb = image_rgb.astype(np.float32)
        image_rgb = (image_rgb - min_chn) * var_reci_chn
        image_rgb = image_rgb.transpose([2, 0, 1]).copy()
        self.image_bytes = np.frombuffer(image_rgb.tobytes())

    # 推理方法
    def inference(self):
        if self.runMode_ == ACL_DEVICE:
            kind = ACL_MEMCPY_DEVICE_TO_DEVICE
        else:
            kind = ACL_MEMCPY_HOST_TO_DEVICE
        if "bytes_to_ptr" in dir(acl.util):
            bytes_data = self.image_bytes.tobytes()
            ptr = acl.util.bytes_to_ptr(bytes_data)
        else:
            ptr = acl.util.numpy_to_ptr(self.image_bytes)
        ret = acl.rt.memcpy(self.input_buffer, self.input_buffer_size, ptr, self.input_buffer_size, kind)
        if ret != ACL_SUCCESS:
            print('memcpy failed, errorCode is', ret)

        ret = acl.mdl.execute(self.model_id, self.input_dataset, self.output_dataset)
        if ret != ACL_SUCCESS:
            print('execute failed, errorCode is', ret)

    # 获取结果
    def get_result(self):
        output_index = 0
        output_data_buffer = acl.mdl.get_dataset_buffer(self.output_dataset, output_index)
        output_data_buffer_addr = acl.get_data_buffer_addr(output_data_buffer)
        output_data_size = acl.get_data_buffer_size(output_data_buffer)
        ptr, ret = acl.rt.malloc_host(output_data_size)

        if self.runMode_ == ACL_DEVICE:
            kind = ACL_MEMCPY_DEVICE_TO_HOST
        else:
            kind = ACL_MEMCPY_HOST_TO_HOST
        ret = acl.rt.memcpy(ptr, output_data_size, output_data_buffer_addr, output_data_size, ACL_MEMCPY_HOST_TO_DEVICE)
        if ret != ACL_SUCCESS:
            print('memcpy failed, errorCode is', ret)

        index = 0
        dims, ret = acl.mdl.get_cur_output_dims(self.model_desc, index)
        if ret != ACL_SUCCESS:
            print('get output dims failed, errorCode is', ret)
        out_dim = dims['dims']

        if "ptr_to_bytes" in dir(acl.util):
            bytes_data = acl.util.ptr_to_bytes(ptr, output_data_size)
            data = np.frombuffer(bytes_data, dtype=np.float32).reshape(out_dim)
        else:
            data = acl.util.ptr_to_numpy(ptr, out_dim, NPY_FLOAT32)

        data = data.flatten()
        vals = np.exp(data) / np.sum(np.exp(data))
        top_index = vals.argsort()[-1]
        label_string = label.get(str(top_index))
        assert label_string, "the key of label is not exist"
        label_class = ",".join(label_string)
        text = f"label:{top_index}  conf:{vals[top_index]:06f}  class:{label_class}"

        ret = acl.rt.free_host(ptr)
        if ret != ACL_SUCCESS:
            print('free host failed, errorCode is', ret)

        return text

    # 释放资源
    def release_resource(self):
        acl.rt.free(self.input_buffer)
        acl.mdl.destroy_dataset(self.input_dataset)
        acl.rt.free(self.output_buffer)
        acl.mdl.destroy_dataset(self.output_dataset)
        ret = acl.mdl.unload(self.model_id)
        if ret != ACL_SUCCESS:
            print('unload model failed, errorCode is', ret)

        if self.model_desc:
            acl.mdl.destroy_desc(self.model_desc)
            self.model_desc = None

        if self.stream:
            ret = acl.rt.destroy_stream(self.stream)
            if ret != ACL_SUCCESS:
                print('destroy stream failed, errorCode is', ret)
            self.stream = None

        if self.context:
            ret = acl.rt.destroy_context(self.context)
            if ret != ACL_SUCCESS:
                print('destroy context failed, errorCode is', ret)
            self.context = None

        ret = acl.rt.reset_device(self.device_id)
        if ret != ACL_SUCCESS:
            print('reset device failed, errorCode is', ret)
            
        ret = acl.finalize()
        if ret != ACL_SUCCESS:
            print('finalize failed, errorCode is', ret)

# 定义HTTP请求处理器
class RequestHandler(BaseHTTPRequestHandler):
        # 处理POST请求
    def do_POST(self):
        # 解析form-data数据
        content_type, _ = cgi.parse_header(self.headers['Content-Type'])
        if content_type == 'multipart/form-data':
            form = cgi.FieldStorage(
                fp=self.rfile,
                headers=self.headers,
                environ={'REQUEST_METHOD': 'POST',
                         'CONTENT_TYPE': self.headers['Content-Type'],}
            )
            # 提取图像文件
            if 'image' in form:
                file_item = form['image']
                image_data = file_item.file.read()
                # 调用模型处理图片
                response = self.server.model_handler(image_data)
                
                # 格式化响应为JSON
                result = {
                    "result": response  # 假设 response 是 "label:162  conf:0.908805  class:beagle"
                }
                response_json = json.dumps(result)
                
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(response_json.encode())
            else:
                self.send_error(400, "File not found in form-data")
        else:
            self.send_error(400, "Unsupported content type")

# 启动HTTP服务器
def run_server():
    device = 0
    model_width = 224
    model_height = 224
    current_dir = os.path.dirname(os.path.abspath(__file__))
    model_path = os.path.join(current_dir, "../model/resnet50.om")

    if not os.path.exists(model_path):
        raise Exception("the model does not exist")

    # 初始化模型
    net = Sample_resnet_quick_start(device, model_path, model_width, model_height)
    net.init_resource()

    # 定义处理函数
    def model_handler(image_data):
        net.process_input(image_data)
        net.inference()
        result = net.get_result()
        return result

    # 启动HTTP服务器
    server_address = ('', 5000)
    httpd = HTTPServer(server_address, RequestHandler)
    httpd.model_handler = model_handler
    print('Starting server on port 5000...')
    httpd.serve_forever()

if __name__ == '__main__':
    run_server()
