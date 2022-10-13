import re
import logging

from paddleocr import PaddleOCR

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
logger = logging.getLogger(__name__)
class Inference:
    def run() -> str:
        ocr = PaddleOCR(use_angle_cls=False, use_gpu=True, lang="en")  # need to run only once to download and load model into memory
        result = ocr.ocr("/opt/nvidia/deepstream/deepstream-6.0/sources/apps/sample_apps/deepstream-src-appsrc/OCR/images/ocr.jpg", cls = False)
        for line in result:
            ocr_string = str.strip(line[1][0])
            if "USDOT" in ocr_string or "US DOT" in ocr_string:
                ocr_string = re.sub("[a-zA-Z]", "", ocr_string)
                ocr_result = str.strip(ocr_string.replace("#",""))
                if len(ocr_result) == 6 or len(ocr_result) == 8:
                    return str.strip(ocr_string.replace("#",""))
            else:
                continue
        return ""
