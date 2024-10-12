构建

```bash
docker build -t video_transcoder_multiway . --build-arg "http_proxy=http://192.168.58.1:2080" --build-arg "https_proxy=http://192.168.58.1:2080"
```

启动

```bash
docker run -it --privileged \
    --device=/dev/dri \
    --device=/dev/dma_heap \
    --device=/dev/rga \
    --device=/dev/mpp_service \
    -p 8081:8080 \
    --name=video_transcoder_multiway \
    video_transcoder_multiway:latest
```

```
docker exec -it video_transcoder_multiway /bin/bash
```

停止

```bash
docker stop video_transcoder_multiway && docker rm video_transcoder_multiway
```

开始处理视频请求：

```
POST http://192.168.137.2:8081/start?url=xxx?bit_rate=400000
```

响应：

```
成功：
{
    "status": "success"
}
失败：
{
    "status": "failed",
    "reason": "<reason>"
}
```

停止处理视频请求：

```
POST http://192.168.137.2:8081/stop?url=xxx
```

响应：

```
成功：
{
    "status": "success"
}
失败：
{
    "status": "failed",
    "reason": "<reason>"
}
```


