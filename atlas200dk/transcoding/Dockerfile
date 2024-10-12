FROM openeuler-22.03-lts-sp3

# 备份并替换YUM仓库文件
RUN mv /etc/yum.repos.d/openEuler.repo /etc/yum.repos.d/openEuler.repo.bak
COPY repo.txt /etc/yum.repos.d/openEuler.repo

# 安装必要的工具，包括cmake和opencv
RUN yum install -y python3-pip && yum install -y mesa-libGL
RUN pip3 config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple && \
pip3 install "numpy<2" && pip3 install opencv-python 
COPY src /transcodeing/src
COPY data /transcodeing/data
COPY acl /usr/local/Ascend/ascend-toolkit/7.0.RC1/aarch64-linux/include/acl

# 设置Ascend环境变量
ENV ASCEND_TOOLKIT_HOME=/usr/local/Ascend/ascend-toolkit/latest \
LD_LIBRARY_PATH=/usr/local/Ascend/ascend-toolkit/latest/lib64:/usr/local/Ascend/ascend-toolkit/latest/lib64/plugin/opskernel:/usr/local/Ascend/ascend-toolkit/latest/lib64/plugin/nnengine:/usr/local/Ascend/ascend-toolkit/latest/x86_64-linux/devlib/:/usr/local/Ascend/driver/lib64:${LD_LIBRARY_PATH} \
PYTHONPATH=/usr/local/Ascend/ascend-toolkit/latest/python/site-packages:/usr/local/Ascend/ascend-toolkit/latest/opp/op_impl/built-in/ai_core/tbe:$THIRDPART_PATH/acllite:$PYTHONPATH \
PATH=/usr/local/Ascend/ascend-toolkit/latest/bin:/usr/local/Ascend/ascend-toolkit/latest/compiler/ccec_compiler/bin:$PATH \
ASCEND_AICPU_PATH=/usr/local/Ascend/ascend-toolkit/latest \
ASCEND_OPP_PATH=/usr/local/Ascend/ascend-toolkit/latest/opp \
TOOLCHAIN_HOME=/usr/local/Ascend/ascend-toolkit/latest/toolkit \
ASCEND_HOME_PATH=/usr/local/Ascend/ascend-toolkit/latest \
TMOUT=3600

# 设置工作目录
WORKDIR /transcodeing/src
# 运行文字分类程序
# CMD ["python3", "transcodeing.py"]


