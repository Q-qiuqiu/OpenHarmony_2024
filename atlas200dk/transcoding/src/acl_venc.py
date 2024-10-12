# -*- coding:utf-8 -*-
import os
import cv2  # 使用OpenCV拉取视频流
import numpy as np
import acl
import time
# enum
memcpy_kind = {
    "ACL_MEMCPY_HOST_TO_HOST": 0,
    "ACL_MEMCPY_HOST_TO_DEVICE": 1,
    "ACL_MEMCPY_DEVICE_TO_HOST": 2,
    "ACL_MEMCPY_DEVICE_TO_DEVICE": 3
}

"""
enType 0
0 H265 main level
1 H264 baseline level
2 H264 main level
3 H264 high level
"""

VENC_ENTYPE = 2

"""
format 2
1 YUV420 semi-planner (nv12)
2 YVU420 semi-planner (nv21)
"""

VENC_FORMAT = 1
DEVICE_ID = 0

STREAM_URL  = "rtsp://192.168.137.1:8554/video"  # RTSP视频流地址

def check_ret(message, ret_int):
    """
    功能简介：检测pyACL函数返回值是否正常，如果非0则会抛出异常
    参数：ret，pyACL函数返回值
    返回值：无
    """
    if ret_int != 0:
        raise Exception("{} failed ret_int={}".format(message, ret_int))


def check_none(message, ret_none):
    """
    功能简介：检测值是否为空，如果为空则会抛出异常
    参数：ret_none
    返回值：无
    """
    if ret_none is None:
        raise Exception("{} failed"
                        .format(message))


class AclVenc(object):
    # 视频编码
    def __init__(self):
        self.input_stream_mem = None
        self.venc_channel_desc = 0
        self.dvpp_pic_desc = 0
        self.frame_config = None
        self.cb_thread_id = None
        self.pic_host = None
        self.context, ret = acl.rt.create_context(DEVICE_ID)
        check_ret("acl.rt.create_context", ret)
        self.callback_run_flag = True
        self.stream_host = None
        if os.path.exists('../data/output.h264'):
             os.remove('../data/output.h264')

    def cb_thread_func(self, args_list):
        context = args_list[0]
        timeout = args_list[1]
        print("[INFO] cb_thread_func args_list = ", context, timeout,
              self.callback_run_flag)

        context, ret = acl.rt.create_context(DEVICE_ID)
        if ret != 0:
            print("[INFO] cb_thread_func acl.rt.create_context ret=", ret)
            return
        while self.callback_run_flag is True:
            ret = acl.rt.process_report(timeout)

        ret = acl.rt.destroy_context(context)
        print("[INFO] cb_thread_func acl.rt.destroy_context ret=", ret)

    def release_resource(self):
        print("[INFO] free resource")
        if self.stream_host:
            acl.rt.free_host(self.stream_host)
        if self.pic_host:
            acl.rt.free_host(self.pic_host)
        if self.input_stream_mem:
            acl.media.dvpp_free(self.input_stream_mem)
        if self.venc_channel_desc != 0:
            acl.media.venc_destroy_channel_desc(self.venc_channel_desc)
        if self.dvpp_pic_desc != 0:
            acl.media.dvpp_destroy_pic_desc(self.dvpp_pic_desc)
        if self.frame_config:
            acl.media.venc_destroy_frame_config(self.frame_config)
        acl.rt.destroy_context(self.context)

    def venc_init(self):
        self.venc_channel_desc = acl.media.venc_create_channel_desc()
        check_none("acl.media.venc_create_channel_desc",
                   self.venc_channel_desc)

    def callback_func(self, input_pic_desc, output_stream_desc, user_data):
        if output_stream_desc == 0:
            print("[INFO] [venc] output_stream_desc is null")
            return
        stream_data = acl.media.dvpp_get_stream_desc_data(output_stream_desc)
        if stream_data is None:
            print("[INFO] [venc] acl.media.dvpp_get_stream_desc_data is none")
            return
        ret = acl.media.dvpp_get_stream_desc_ret_code(output_stream_desc)
        if ret == 0:
            stream_data_size = acl.media.dvpp_get_stream_desc_size(
                output_stream_desc)
            
            print("[INFO] [venc] stream_data size", stream_data_size)


            # stream memcpy d2h
            if "bytes_to_ptr" in dir(acl.util):
                data = bytes(stream_data_size)
                data_ptr = acl.util.bytes_to_ptr(data)
            else:
                data = np.zeros(stream_data_size, dtype=np.byte)
                data_ptr = acl.util.numpy_to_ptr(data)
            ret = acl.rt.memcpy(data_ptr, stream_data_size,
                                stream_data, stream_data_size,
                                memcpy_kind.get("ACL_MEMCPY_DEVICE_TO_HOST"))
            if ret != 0:
                print("[INFO] [venc] acl.rt.memcpy ret=", ret)
                return
                
            # 将数据写入文件
            output_path = '../data/output.h264'  # 指定输出文件路径
            with open(output_path, 'ab') as f:
                f.write(data)


    def venc_set_desc(self, width, height,fps,bitrate):
        # 编码器通道设置
        acl.media.venc_set_channel_desc_thread_id(self.venc_channel_desc, self.cb_thread_id)
        acl.media.venc_set_channel_desc_callback(self.venc_channel_desc, self.callback_func)
        acl.media.venc_set_channel_desc_entype(self.venc_channel_desc, VENC_ENTYPE)
        acl.media.venc_set_channel_desc_pic_format(self.venc_channel_desc, VENC_FORMAT)
        key_frame_interval = fps  # 设置关键帧间隔为帧率的值
        acl.media.venc_set_channel_desc_key_frame_interval(self.venc_channel_desc, key_frame_interval)
        acl.media.venc_set_channel_desc_pic_height(self.venc_channel_desc, height)
        acl.media.venc_set_channel_desc_pic_width(self.venc_channel_desc, width)
        # 添加码率设置
        acl.media.venc_set_channel_desc_param(self.venc_channel_desc, 11, bitrate)  # 设置码率

    def venc_set_frame_config(self, frame_confg, eos, iframe):
        acl.media.venc_set_frame_config_eos(frame_confg, eos)
        acl.media.venc_set_frame_config_force_i_frame(frame_confg, iframe)
    def venc_get_frame_config(self, frame_confg):
        get_eos = acl.media.venc_get_frame_config_eos(frame_confg)
        check_ret("acl.media.venc_get_frame_config_eos", get_eos)
        get_force_frame = acl.media.venc_get_frame_config_force_i_frame(
            frame_confg)
        check_ret("acl.media.venc_get_frame_config_force_i_frame",
                  get_force_frame)

    def mergeUV(self,u, v):
        if u.shape == v.shape:
            uv = np.zeros(shape=(u.shape[0], u.shape[1] * 2), dtype=u.dtype)
            for i in range(0, u.shape[0]):
                for j in range(0, u.shape[1]):
                    uv[i, 2 * j] = u[i, j]
                    uv[i, 2 * j + 1] = v[i, j]
            return uv
        else:
            print("Size of Channel U is different from Channel V")
            return None
    def rgb2nv12(self,image):
        if image.ndim == 3:
            b = image[:, :, 0]
            g = image[:, :, 1]
            r = image[:, :, 2]
            
            # 计算 YUV 分量
            y = (0.299 * r + 0.587 * g + 0.114 * b).astype(np.uint8)
            u = (-0.169 * r - 0.331 * g + 0.5 * b + 128)[::2, ::2].astype(np.uint8)
            v = (0.5 * r - 0.419 * g - 0.081 * b + 128)[::2, ::2].astype(np.uint8)

            # 合并 U 和 V 分量
            uv = self.mergeUV(u, v)
            if uv is not None:
                yuv = np.vstack((y, uv))
                return yuv.astype(np.uint8)
        else:
            print("Image is not BGR format")
            return None 
    def venc_process(self, frame, width,height):
        # print("Frame shape:", frame.shape)
        # print("Frame size:", frame.size)

        start_time = time.time()  # 记录开始时间

        # 将 RGB 转换为 NV12
        nv12_frame = self.rgb2nv12(frame)

        # 使用转换后的 NV12 数据
        self.dvpp_pic_desc = acl.media.dvpp_create_pic_desc()
        check_none("acl.media.dvpp_create_pic_desc", self.dvpp_pic_desc)

        frame_ptr = acl.util.numpy_to_ptr(nv12_frame)  # 将 NV12 frame 转换为指针
        ret = acl.media.dvpp_set_pic_desc_data(self.dvpp_pic_desc, frame_ptr)
        ret = acl.media.dvpp_set_pic_desc_size(self.dvpp_pic_desc, nv12_frame.size)  # 使用 NV12 的大小
        print("[INFO] set pic desc size")

        self.venc_set_frame_config(self.frame_config, 0, 0)
        print("[INFO] set frame config")
        self.venc_get_frame_config(self.frame_config)

        # 发送帧
        venc_cnt = 1
        while venc_cnt:
            ret = acl.media.venc_send_frame(self.venc_channel_desc, self.dvpp_pic_desc, 0, self.frame_config, None)
            check_ret("acl.media.venc_send_frame", ret)
            venc_cnt -= 1
        self.venc_set_frame_config(self.frame_config, 1, 0)
        
        end_time = time.time()  # 记录结束时间
        process_time = end_time - start_time  # 计算处理时间
        print(f"[INFO] Frame processed in {process_time:.3f} seconds")
        # 发送结束eos帧
        # print("[INFO] venc send frame eos")
        # ret = acl.media.venc_send_frame(self.venc_channel_desc, 0, 0, self.frame_config, None)
        # print("[INFO] acl.media.venc_send_frame ret=", ret)

        # print("[INFO] venc process success")

    def venc_run(self):
        timeout = 1000
        self.cb_thread_id, ret = acl.util.start_thread(self.cb_thread_func, [self.context, timeout])
        
        # 确保 cb_thread_id 已经初始化为正确的线程 ID
        if self.cb_thread_id is None:
            raise Exception("Failed to initialize thread, cb_thread_id is None")

        check_ret("acl.util.start_thread", ret)
        print("[INFO] start_thread", self.cb_thread_id, ret)

        cap = cv2.VideoCapture(STREAM_URL)  # 拉取 ffmpeg 的视频流,拉取的是BGR/RGB数据
        if not cap.isOpened():
            raise Exception("Failed to open video stream")

        # 获取帧率和分辨率
        fps = int(cap.get(cv2.CAP_PROP_FPS))  # 自动获取帧率
        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))  # 获取视频宽度
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))  # 获取视频高度

        print(f"Video stream properties: FPS = {fps}, Width = {width}, Height = {height}")

        # 初始化编码器设置
        self.venc_set_desc(width, height, fps,4000)
        
        # 初始化编码通道
        ret = acl.media.venc_create_channel(self.venc_channel_desc)
        check_ret("acl.media.venc_create_channel", ret)

        self.frame_config = acl.media.venc_create_frame_config()
        check_none("acl.media.venc_create_frame_config", self.frame_config)

        while cap.isOpened():
            ret, frame = cap.read()
            if not ret:
                print("[INFO] No more frames to read.")
                break
            
            # 传递 frame 和大小
            self.venc_process(frame, width,height)

        # 发送结束eos帧
        print("[INFO] venc send frame eos")
        ret = acl.media.venc_send_frame(self.venc_channel_desc, 0, 0, self.frame_config, None)
        check_ret("acl.media.venc_send_frame eos", ret)
        print("[INFO] venc process success")

        cap.release()

if __name__ == '__main__':
    ret = acl.init("")
    check_ret("acl.init", ret)
    ret = acl.rt.set_device(DEVICE_ID)
    check_ret("acl.rt.set_device", ret)
    venc_handle = AclVenc()
    venc_handle.venc_init()
    venc_handle.venc_run()
    venc_handle.release_resource()
    ret = acl.rt.reset_device(DEVICE_ID)
    check_ret("acl.rt.reset_device", ret)
    ret = acl.finalize()
    check_ret("acl.finalize", ret)
