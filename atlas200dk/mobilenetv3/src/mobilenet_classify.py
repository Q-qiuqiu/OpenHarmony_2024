# -*- coding:utf-8 -*-
import sys
import os
import acl
import image_net_classes
from http.server import BaseHTTPRequestHandler, HTTPServer
import cgi
from io import BytesIO
import shutil

path = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(path, ".."))
sys.path.append(os.path.join(path, "../../../../common/"))
sys.path.append(os.path.join(path, "../../../../common/acllite"))

from constants import ACL_MEM_MALLOC_HUGE_FIRST, ACL_MEMCPY_DEVICE_TO_DEVICE, IMG_EXT
from acllite_imageproc import AclLiteImageProc
from acllite_model import AclLiteModel
from acllite_image import AclLiteImage
from acllite_resource import AclLiteResource
from PIL import Image, ImageDraw, ImageFont

class Classify(object):
    """
	define gesture class
    """
    def __init__(self, acl_resource, model_path, model_width, model_height):
        self._model_path = model_path
        self._model_width = model_width
        self._model_height = model_height
        self._dvpp = AclLiteImageProc(acl_resource)
        self._model = AclLiteModel(model_path)

    def __del__(self):
        if self._dvpp:
            del self._dvpp
        print("[Sample] class Samle release source success")

    def pre_process(self, image):
        """
        pre_precess
        """
        yuv_image = self._dvpp.jpegd(image)
        resized_image = self._dvpp.resize(yuv_image, 
                        self._model_width, self._model_height)
        print("resize yuv end")
        return resized_image

    def inference(self, resized_image):
        """
	    inference
        """
        return self._model.execute([resized_image, ])

    def post_process(self, infer_output):
        data = infer_output[0]
        vals = data.flatten()
        top_k = vals.argsort()[-1:-6:-1]
        results = []
        for n in top_k:
            object_class = image_net_classes.get_image_net_class(n)
            results.append({
                "label": int(n),  # 将 np.int64 转为 int
                "confidence": float(vals[n]),  # 将 np.float32 转为 float
                "class": object_class
            })
        return results

SRC_PATH = os.path.realpath(__file__).rsplit("/", 1)[0]
MODEL_PATH = os.path.join(SRC_PATH, "../model/mobilenet_yuv.om")
MODEL_WIDTH = 224
MODEL_HEIGHT = 224

acl_resource = AclLiteResource()
acl_resource.init()
classify = Classify(acl_resource, MODEL_PATH, MODEL_WIDTH, MODEL_HEIGHT)

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        # Parse the form-data
        ctype, pdict = cgi.parse_header(self.headers['Content-Type'])
        if ctype == 'multipart/form-data':
            form = cgi.FieldStorage(fp=self.rfile, headers=self.headers,
                                    environ={'REQUEST_METHOD': 'POST', 'CONTENT_TYPE': self.headers['Content-Type']})
            
            # Get the file from the form-data
            if 'image' not in form:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"No image part")
                return
            
            file_field = form['image']
            if file_field.filename == '':
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"No selected file")
                return
            
            # Save the uploaded file as a temporary file
            temp_image_path = os.path.join(SRC_PATH, file_field.filename)
            with open(temp_image_path, 'wb') as output_file:
                shutil.copyfileobj(file_field.file, output_file)
            
            # Process the saved image
            image = AclLiteImage(temp_image_path)
            image_dvpp = image.copy_to_dvpp()
            resized_image = classify.pre_process(image_dvpp)
            result = classify.inference(resized_image)
            classification_results = classify.post_process(result)

            # Clean up temporary file
            os.remove(temp_image_path)

            # Return classification results as JSON
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            response = BytesIO()
            response.write(str.encode(str(classification_results)))
            self.wfile.write(response.getvalue())

def run(server_class=HTTPServer, handler_class=SimpleHTTPRequestHandler, port=5001):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f'Starting httpd on port {port}...')
    httpd.serve_forever()

if __name__ == '__main__':
    run()