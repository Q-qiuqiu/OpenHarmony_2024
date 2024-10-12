ffmpeg推流命令
```
ffmpeg -re -stream_loop -1 -i C:\Users\Vic'to'r\Desktop\vbench\videos\crf18\bike_1280x720_29.mkv -c:v libx264 -f rtsp -rtsp_transport tcp rtsp://127.0.0.1:8554/video

```

1. **`-re`** : 这个选项表示以实时速度读取输入文件。这通常用于流媒体应用，确保输入流以正常播放速度进行处理，而不是尽可能快地读取。
2. **`-stream_loop -1`** : 这个选项表示无限循环输入文件（`-1`代表无限次循环）。在此情况下，视频文件将不断重复播放，直到手动停止。
3. **`-i C:\Users\Vic'to'r\Desktop\vbench\videos\crf18\bike_1280x720_29.mkv`** : `-i` 后面跟的是输入文件的路径。这里指定的是一个 MKV 格式的视频文件。
5. **`-c:v libx264`** : 这个选项指定视频编码器为 `libx264`，这是一个流行的 H.264 视频编码库，用于高效地压缩视频。
6. **`-f rtsp`** : 这个选项指定输出格式为 RTSP（实时流传输协议）。RTSP 是用于流媒体传输的网络协议。
7. **`rtsp://127.0.0.1:8554/video`** : 这是输出流的目标地址，表示将视频流发送到本地计算机（`127.0.0.1`）上的 RTSP 服务器，端口号为 `8554`，流的名称为 `/video`。

部署步骤如下：

构建镜像

```
docker build -t transcoding:latest .
```

创建容器

```
docker run -itd --privileged \
    -e ASCEND_VISIBLE_DEVICES=0 \
    -e ASCEND_ALLOW_LINK=True \
    --device=/dev/svm0 \
    --device=/dev/ts_aisle \
    --device=/dev/upgrade \
    --device=/dev/sys \
    --device=/dev/vdec \
    --device=/dev/vpc \
    --device=/dev/pngd \
    --device=/dev/venc \
    --device=/dev/dvpp_cmdlist \
    --device=/dev/log_drv \
    -v /etc/sys_version.conf:/etc/sys_version.conf \
    -v /etc/hdcBasic.cfg:/etc/hdcBasic.cfg \
    -v /usr/local/sbin/npu-smi:/usr/local/sbin/npu-smi \
    -v /usr/local/Ascend/driver/lib64:/usr/local/Ascend/driver/lib64 \
    -v /usr/lib64/aicpu_kernels/:/usr/lib64/aicpu_kernels/ \
    -v /var/slogd:/var/slogd \
    -v /var/dmp_daemon:/var/dmp_daemon \
    -v /usr/lib64/libaicpu_processer.so:/usr/lib64/libaicpu_processer.so \
    -v /usr/lib64/libaicpu_prof.so:/usr/lib64/libaicpu_prof.so \
    -v /usr/lib64/libaicpu_sharder.so:/usr/lib64/libaicpu_sharder.so \
    -v /usr/lib64/libadump.so:/usr/lib64/libadump.so \
    -v /usr/lib64/libtsd_eventclient.so:/usr/lib64/libtsd_eventclient.so \
    -v /usr/lib64/libaicpu_scheduler.so:/usr/lib64/libaicpu_scheduler.so \
    -v /usr/lib64/libdcmi.so:/usr/lib64/libdcmi.so \
    -v /usr/lib64/libmpi_dvpp_adapter.so:/usr/lib64/libmpi_dvpp_adapter.so \
    -v /usr/lib64/libstackcore.so:/usr/lib64/libstackcore.so \
    -v /usr/local/Ascend/driver:/usr/local/Ascend/driver \
    -v /etc/ascend_install.info:/etc/ascend_install.info \
    -v /var/log/ascend_seclog:/var/log/ascend_seclog \
    -v /var/davinci/driver:/var/davinci/driver \
    -v /usr/lib64/libc_sec.so:/usr/lib64/libc_sec.so \
    -v /usr/lib64/libdevmmap.so:/usr/lib64/libdevmmap.so \
    -v /usr/lib64/libdrvdsmi.so:/usr/lib64/libdrvdsmi.so \
    -v /usr/lib64/libslog.so:/usr/lib64/libslog.so \
    -v /usr/lib64/libmmpa.so:/usr/lib64/libmmpa.so \
    -v /usr/lib64/libascend_hal.so:/usr/lib64/libascend_hal.so \
    -v /usr/local/Ascend/ascend-toolkit:/usr/local/Ascend/ascend-toolkit \
    --name transcoding \
    -p 8554:8554 \
    transcoding
```


