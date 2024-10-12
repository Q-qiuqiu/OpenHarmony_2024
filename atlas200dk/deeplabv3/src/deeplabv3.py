# -*- coding: utf-8 -*-

import sys
import os
import cv2 as cv
import numpy as np
from io import BytesIO
from http.server import BaseHTTPRequestHandler, HTTPServer
import acllite_utils as utils
from acllite_imageproc import AclLiteImageProc
import constants as const
from acllite_model import AclLiteModel
from acllite_image import AclLiteImage
from acllite_resource import AclLiteResource
from urllib.parse import parse_qs
import cgi

MODEL_WIDTH = 513
MODEL_HEIGHT = 513

SRC_PATH = os.path.realpath(__file__).rsplit("/", 1)[0]
MODEL_PATH = os.path.join(SRC_PATH, "../model/deeplabv3_plus.om")
# 全局变量：ACL资源和模型
acl_resource = None
model = None
def preprocess(image_data):
    """preprocess"""
    # 读取图像
    bgr_img = cv.imdecode(np.frombuffer(image_data, np.uint8), cv.IMREAD_COLOR)
    orig_shape = bgr_img.shape[:2]
    # 调整图像大小
    img = cv.resize(bgr_img, (MODEL_WIDTH, MODEL_HEIGHT)).astype(np.int8)
    if not img.flags['C_CONTIGUOUS']:
        img = np.ascontiguousarray(img)
    return orig_shape, img

def postprocess(result_list, orig_shape):
    """postprocess"""
    result_img = result_list[0].reshape(513, 513)
    result_img = result_img.astype('uint8')
    img = cv.merge((result_img, result_img, result_img))
    bgr_img = cv.resize(img, (orig_shape[1], orig_shape[0]))
    bgr_img = (bgr_img + 255)
    # 返回处理后的图像
    _, encoded_img = cv.imencode('.jpg', bgr_img)
    return encoded_img.tobytes()

class RequestHandler(BaseHTTPRequestHandler):
     def do_POST(self):
        # 仅在路径为 /segmentation 时处理请求
        if self.path == "/segmentation":
            # 设置表单解析
            ctype, pdict = cgi.parse_header(self.headers.get('Content-Type'))
            if ctype == 'multipart/form-data':
                pdict['boundary'] = bytes(pdict['boundary'], "utf-8")
                fields = cgi.parse_multipart(self.rfile, pdict)

                # 从请求中获取图像文件
                if 'image' in fields:
                    image_data = fields['image'][0]

                     # 使用全局初始化的ACL资源和模型
                    global acl_resource, model


                    # 处理图像
                    orig_shape, l_data = preprocess(image_data)
                    result_list = model.execute([l_data])
                    processed_image = postprocess(result_list, orig_shape)

                    # 发送响应头
                    self.send_response(200)
                    self.send_header('Content-type', 'image/jpeg')
                    self.end_headers()
                    
                    # 返回处理后的图像
                    self.wfile.write(processed_image)
                else:
                    self.send_error(400, "Field 'image' is required")
            else:
                self.send_error(400, "Content-Type must be multipart/form-data")
        else:
            self.send_error(404, "Not Found: Only /segmentation is supported")

def run(server_class=HTTPServer, handler_class=RequestHandler, port=6001):
    global acl_resource, model

    # 只在服务器启动时初始化一次ACL资源和模型
    acl_resource = AclLiteResource()
    acl_resource.init()
    model = AclLiteModel(MODEL_PATH)
    
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f"Starting server on port {port}")
    httpd.serve_forever()


if __name__ == '__main__':
    run()