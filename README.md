# USDOT
USDOT

## Jetson USDOT Install Process
### 1.  
sudo apt-get install libcurl4-openssl-dev

### 2.  
https://www.paddlepaddle.org.cn/inference/user_guides/download_lib.html#python  
Jetpack4.6.1：nv_jetson-cuda10.2-trt8.2-xavier

### 3.  
python3 -m pip install --upgrade pip

### 4.  
python3 -m pip install -U paddlepaddle_gpu-2.3.2-cp37-cp37m-linux_aarch64.whl

### 5.  
git clone https://github.com/PaddlePaddle/PaddleOCR  
cd PaddleOCR/  
python3 -m pip install -r requirements.txt  
pip3 install paddleocr

### 6.  
cd deepstream-src-appsrc/OCR/  
python3 -m pip install -r requirements.txt


## Jetson USDOT Python Site-Packages

### Module  
shapely  
scikit-image  
imgaug  
pyclipper  
lmdb  
tqdm  
numpy  
visualdl  
rapidfuzz  
opencv-contrib-python  
cython  
lxml  
premailer  
openpyxl  
attrdict  
fastapi  
uvicorn  

### Model
