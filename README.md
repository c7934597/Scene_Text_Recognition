# USDOT
USDOT

## 1. Install Process
### 1-1.  
sudo apt-get install libcurl4-openssl-dev

### 1-2.  
https://www.paddlepaddle.org.cn/inference/user_guides/download_lib.html#python  
Jetpack4.6.1：nv_jetson-cuda10.2-trt8.2-xavier

### 1-3.  
python3 -m pip install --upgrade pip

### 1-4.  
python3 -m pip install -U paddlepaddle_gpu-2.3.2-cp37-cp37m-linux_aarch64.whl

### 1-5.  
git clone https://github.com/PaddlePaddle/PaddleOCR  
cd PaddleOCR/  
python3 -m pip install -r requirements.txt  
pip3 install paddleocr

### 1-6.  
cd deepstream-src-appsrc/OCR/  
python3 -m pip install -r requirements.txt


## 2. Deploye Python Site-Packages & Models  
### 2-1. Site-Packages  
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

### 2-2. Models  
Git has a paddleocr folder, please move it to jetson root.

## 3. Change ocr image file path  
"/opt/nvidia/deepstream/deepstream-6.0/sources/apps/sample_apps/deepstream-src-appsrc/OCR/images/ocr.jpg"  
  
to  

"/New Path/deepstream-src-appsrc/OCR/images/ocr.jpg"