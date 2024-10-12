构建

```bash
docker build -t resnet_infer_rk3588 . --build-arg "http_proxy=http://192.168.58.1:2080" --build-arg "https_proxy=http://192.168.58.1:2080"
```

启动

```bash
docker run -it --privileged \
    -p 8090:8080 \
    --name=resnet_infer_rk3588 \
    resnet_infer_rk3588:latest
```

```
docker exec -it resnet_infer_rk3588 /bin/bash
```

停止

```bash
docker stop resnet_infer_rk3588 && docker rm resnet_infer_rk3588
```
