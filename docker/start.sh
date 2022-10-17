#!/bin/bash
sudo docker run -itd --gpus all --runtime=nvidia -p 8000:8000 -v $PWD/../deepstream-src-appsrc/OCR/images:/deepstream/OCR/images deepstream
