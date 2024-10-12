构建

```bash
docker build -t video_decoder_multiway . --build-arg "http_proxy=http://192.168.58.1:2080" --build-arg "https_proxy=http://192.168.58.1:2080"
```

启动

```bash
docker run -itd --privileged \
    --device=/dev/dri \
    --device=/dev/dma_heap \
    --device=/dev/rga \
    --device=/dev/mpp_service \
    -p 8081:8080 \
    --name=video_decoder_multiway \
    video_decoder_multiway:latest
```

```
docker exec -it video_decoder_multiway /bin/bash
```

停止

```bash
docker stop video_decoder_multiway && docker rm video_decoder_multiway
```

开始处理视频请求：

```
POST http://192.168.137.2:8081/start?url=xxx
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

获取视频帧请求：

```
GET http://192.168.137.2:8081/video?url=xxx
```

响应：

```
总体形式为：
json数据+!字符+二进制视频帧（BGRA8888格式）
```

json数据：

1. 成功

```
{
    "result": [
        [
            269.28125,
            197.75,
            392.71875,
            359.25,
            0.7147121429443359,
            41
        ],
        [
            596.15625,
            246.5625,
            639.84375,
            357.9375,
            0.49903178215026855,
            24
        ],
        [
            15.8125,
            295.171875,
            88.0625,
            358.328125,
            0.27820515632629395,
            41
        ],
        [
            444.40625,
            299.125,
            531.59375,
            360.375,
            0.27074623107910156,
            64
        ]
    ],
    "status": "success",
    "width": 480,
    "height": 360
}
```

成功但没有推理结果：

```
{
    "result": [],
    "status": "success",
    "width": 480,
    "height": 360
}
```

失败：

```
{
    "status": "failed",
    "reason": "<reason>"
}
```
