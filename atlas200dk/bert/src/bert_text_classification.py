# -*- coding: utf-8 -*-
import numpy as np
import os
import codecs
from tokenizer import Tokenizer
import time
import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
from io import BytesIO
import cgi

# 路径设置
path = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(path, ".."))
sys.path.append(os.path.join(path, "../acllite"))

from acllite_resource import AclLiteResource
from acllite_model import AclLiteModel

# 模型和数据文件路径
model_path = '../model/bert_text_classification.om'
dict_path = '../scripts/vocab.txt'
label_path = '../scripts/label.json'
maxlen = 298
token_dict = {}

# 自定义的Tokenizer类
class OurTokenizer(Tokenizer):
    """
    tokenizer text to ID
    """
    def _tokenize(self, text):
        R = []
        for c in text:
            if c in self._token_dict:
                R.append(c)
            else:
                R.append('[UNK]')   # 其他字符用[UNK]表示
        return R

# 文本预处理
def preprocess(text):

    # 将文本转为ID序列，不足maxlen的部分填充0

    tokenizer = OurTokenizer(token_dict)

    # Tokenize
    text = text[:maxlen]
    x1, x2 = tokenizer.encode(first=text)
    X1 = x1 + [0] * (maxlen - len(x1)) if len(x1) < maxlen else x1
    X2 = x2 + [0] * (maxlen - len(x2)) if len(x2) < maxlen else x2
    return X1, X2

# 后处理，选择最大置信度的类别
def postprocess(result_list):
    y = np.argmax(result_list[0])
    return y

# 分类核心逻辑
def classify_text(text, model, label_dict):
    # 执行文本分类并返回分类结果

    X1, X2 = preprocess(text)
    X1 = np.ascontiguousarray(X1, dtype='float32')
    X2 = np.ascontiguousarray(X2, dtype='float32')

    X1 = np.expand_dims(X1, 0)
    X2 = np.expand_dims(X2, 0)

    # 开始计时
    start_time = time.time()
    result_list = model.execute([X1, X2])
    end_time = time.time()

    # 计算分类耗时
    classification_time = end_time - start_time
    y = postprocess(result_list)
    return label_dict.get(str(y), "Unknown"), classification_time

# HTTP请求处理
class RequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):

        # 处理POST请求

        # 仅处理路径为 /textclassify 的请求
        if self.path != "/textclassify":
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Path not found")
            return

        # 获取并解析form-data数据
        content_type, pdict = cgi.parse_header(self.headers['Content-Type'])
        if content_type == 'multipart/form-data':
            pdict['boundary'] = bytes(pdict['boundary'], "utf-8")
            pdict['CONTENT-LENGTH'] = int(self.headers['Content-Length'])
            form = cgi.parse_multipart(self.rfile, pdict)
            text = form.get("text", [""])[0]  # 获取文本参数
        else:
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"Content-Type must be multipart/form-data")
            return

        if not text:
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"Missing text parameter")
            return

        # 分类并返回结果
        label, classification_time = classify_text(text, model, label_dict)
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        response = json.dumps({
            "text": text, 
            "prediction_label": label, 
            "classification_time": classification_time
        })
        self.wfile.write(response.encode())

# 主函数
def main():

    # 初始化acl资源并加载模型和词典

    global model, label_dict

    # ACL资源初始化
    acl_resource = AclLiteResource()
    acl_resource.init()
    model = AclLiteModel(model_path)

    # 加载词典文件
    with codecs.open(dict_path, 'r', 'utf-8') as reader:
        for line in reader:
            token = line.strip()
            token_dict[token] = len(token_dict)

    # 加载标签文件
    with open(label_path, "r", encoding="utf-8") as f:
        label_dict = json.loads(f.read())

    # 启动HTTP服务器
    server_address = ('', 6000)  # 监听6000端口
    httpd = HTTPServer(server_address, RequestHandler)
    print("Server running on port 6000...")
    httpd.serve_forever()

if __name__ == '__main__':
    main()
