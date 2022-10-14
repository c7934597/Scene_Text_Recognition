# Version: 0.1.0
# FROM arm64v8/ubuntu:18.04
# FROM nvcr.io/nvidia/l4t-base:r32.7.1-py3
# FROM platerecognizer/alpr-jetson
FROM nvcr.io/nvidia/l4t-tensorrt:r8.2.1-runtime

ARG OCR_PATH=/deepstream/OCR \
    PADDLEOCR_VERSION=2.6 \
    PYMUDF_VERSION=1.20.3

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8 \
    IMAGE_PATH=$OCR_PATH/images 

RUN apt update \
    && apt install -y htop wget curl git software-properties-common libcurl4-openssl-dev python3.7 python3-pip libgeos-dev

RUN update-alternatives --install /usr/bin/python python /usr/bin/python2.7 1 \
    && update-alternatives --install /usr/bin/python python /usr/bin/python3.7 2 \
    && update-alternatives --install /usr/bin/python python /usr/bin/python3.8 3 \
    && update-alternatives --remove python /usr/bin/python2.7 \
    && update-alternatives --remove python /usr/bin/python3.8 

RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.7 1 \
    && update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 2 \
    && update-alternatives --remove python3 /usr/bin/python3.8 \
    && python3 --version

RUN apt update \    
    && python3.7 -m pip install --upgrade pip wheel \
    && python3.7 -m pip install --upgrade setuptools swig func-timeout \
    && apt install -y python3-distutils python3-apt python3.7-dev libfreetype6 libfreetype6-dev

# RUN pip3 install paddlehub --upgrade

# for amd64 os
# RUN python3 -m pip install paddlepaddle-gpu==2.3.2 

# python 3.6
# https://paddle-inference-lib.bj.bcebos.com/2.4.0-rc0/python/Jetson/jetpack4.6_gcc7.5/xavier/paddlepaddle_gpu-2.4.0rc0-cp36-cp36m-linux_aarch64.whl

# RUN curl -s https://paddle-inference-lib.bj.bcebos.com/2.4.0-rc0/python/Jetson/jetpack4.6_gcc7.5/xavier/paddlepaddle_gpu-2.4.0rc0-cp36-cp36m-linux_aarch64.whl -o paddlepaddle_gpu-2.4.0rc0-cp36-cp36m-linux_aarch64.whl \
#     && python3 -m pip install -U paddlepaddle_gpu-2.4.0rc0-cp36-cp36m-linux_aarch64.whl \
#     && rm -rf paddlepaddle_gpu-2.4.0rc0-cp36-cp36m-linux_aarch64.whl

# python 3.7
# https://paddle-inference-lib.bj.bcebos.com/2.4.0-rc0/python/Jetson/jetpack4.6.1_gcc7.5/xavier/paddlepaddle_gpu-2.4.0rc0-cp37-cp37m-linux_aarch64.whl

RUN curl -s https://paddle-inference-lib.bj.bcebos.com/2.4.0-rc0/python/Jetson/jetpack4.6.1_gcc7.5/xavier/paddlepaddle_gpu-2.4.0rc0-cp37-cp37m-linux_aarch64.whl -o paddlepaddle_gpu-2.4.0rc0-cp37-cp37m-linux_aarch64.whl \
    && python3 -m pip install -U paddlepaddle_gpu-2.4.0rc0-cp37-cp37m-linux_aarch64.whl \
    && rm -rf paddlepaddle_gpu-2.4.0rc0-cp37-cp37m-linux_aarch64.whl

# RUN curl -s https://mupdf.com/downloads/archive/mupdf-$PYMUDF_VERSION-source.tar.gz -o mupdf-$PYMUDF_VERSION-source.tar.gz \
#     && tar zxvf mupdf-$PYMUDF_VERSION-source.tar.gz \
#     && cd mupdf-$PYMUDF_VERSION-source \
#     && make HAVE_X11=no HAVE_GLUT=no prefix=/usr/local install \
#     && rm -rf mupdf-$PYMUDF_VERSION-source.tar.gz  

ADD ./PaddleOCR /PaddleOCR 

# RUN git clone -n https://github.com/PaddlePaddle/PaddleOCR.git --depth 1 /PaddleOCR \
#     && cd /PaddleOCR  \
#     git checkout HEAD requirements.txt 

RUN cd /PaddleOCR \
    && pip3 install -r requirements.txt \
    && pip3 install paddleocr==$PADDLEOCR_VERSION \
    && rm -rf /PaddleOCR 

# RUN git clone  -b 'v2.6.0' --single-branch https://github.com/PaddlePaddle/PaddleOCR.git /PaddleOCR \
#     && cd /PaddleOCR \
#     && python3 -m pip install -r requirements.txt \
#     && pip3 install paddleocr

ADD deepstream-src-appsrc/OCR /deepstream/OCR
ADD .paddleocr /root/./paddleocr

WORKDIR $OCR_PATH

RUN python3 -m pip install -r requirements.txt
RUN ln -sf /usr/lib/aarch64-linux-gnu/libcudnn.so.8.2.1 /usr/lib/libcudnn.so \
    && ln -sf /usr/local/cuda-10.2/targets/aarch64-linux/lib/libcublas.so.10.2.3.300 /usr/lib/libcublas.so

EXPOSE 8868 8000

CMD python3 api.py 
