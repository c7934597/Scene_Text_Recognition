import re
import logging

from paddleocr import PaddleOCR

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
logger = logging.getLogger(__name__)
class Inference:
    def run(image_path) -> str:
        ocr = PaddleOCR(use_angle_cls=False, use_gpu=True, lang="en")  # need to run only once to download and load model into memory
        result = ocr.ocr(image_path, cls = False)
        for line in result:
            if "USDOT" in line[1][0] or "US DOT" in line[1][0] or "U.S.DOT" in line[1][0] or "U.S. DOT" in line[1][0]:
                ocr_result = str.strip(re.sub("[a-zA-Z#.]", "", line[1][0]))
                if 6 <= len(ocr_result) <= 8:
                    return ocr_result
                else:
                    return ""
        return ""
