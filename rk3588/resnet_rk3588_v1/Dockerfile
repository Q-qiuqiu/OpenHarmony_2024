FROM dhub.kubesre.xyz/ubuntu:22.04 AS builder

RUN sed -i 's@//ports.ubuntu.com@//mirrors.aliyun.com@g' /etc/apt/sources.list

RUN apt-get update

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata && \
    ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime && \
    dpkg-reconfigure --frontend noninteractive tzdata

RUN apt-get install -y \
    build-essential \
    cmake \
    libopencv-dev

COPY . /app
WORKDIR /app

RUN mkdir build && cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/app/install && \
    make -j $(nproc) && \
    make install

FROM dhub.kubesre.xyz/ubuntu:22.04

RUN sed -i 's@//ports.ubuntu.com@//mirrors.aliyun.com@g' /etc/apt/sources.list

RUN apt-get update

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata && \
    ln -fs /usr/share/zoneinfo/Asia/Shanghai /etc/localtime && \
    dpkg-reconfigure --frontend noninteractive tzdata

RUN apt-get install -y \
    libopencv-dev

COPY --from=builder /app/install /app
ENV LD_LIBRARY_PATH=/app/lib:$LD_LIBRARY_PATH

WORKDIR /app

ENTRYPOINT ["bin/resnet_infer"]
