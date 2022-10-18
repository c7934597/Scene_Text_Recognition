import os
import re
import logging

from paddleocr import PaddleOCR

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
logger = logging.getLogger(__name__)
class Inference:
    def run() -> str:
        if "IMAGE_PATH" in os.environ:
            if os.environ["IMAGE_PATH"] == "undefined" or os.environ["IMAGE_PATH"] == "":
                logger.error("Environment Variable - IMAGE_PATH value is error")
                os._exit(0)
            else:
                IMAGE_PATH = os.environ["IMAGE_PATH"]
        ocr = PaddleOCR(use_angle_cls=False, use_gpu=True, lang="en")  # need to run only once to download and load model into memory
        result = ocr.ocr(IMAGE_PATH, cls = False)
        for line in result:
            if "USDOT" in line[1][0] or "US DOT" in line[1][0] or "U.S.DOT" in line[1][0] or "U.S. DOT" in line[1][0]:
                ocr_string = re.sub("[a-zA-Z#.]", "", ocr_string)
                ocr_result = str.strip(ocr_string)
                if 6 <= len(ocr_result) <= 8:
                    return ocr_result
                else:
                    return ""
        return ""
