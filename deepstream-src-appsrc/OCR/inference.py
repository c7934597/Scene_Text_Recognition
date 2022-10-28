import re
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
logger = logging.getLogger(__name__)
class Inference:
    def run(ocr, image_path) -> str:
        result = ocr.ocr(image_path, cls = False)
        for sub_result in result:
            for line in sub_result:
                ocr_result = line[1][0]
                if "DOT" in ocr_result:
                    ocr_result = str.strip(re.sub("[a-zA-Z#-.]", "", ocr_result))
                    if 6 <= len(ocr_result) <= 8:
                        return ocr_result
                    else:
                        return ""
        return ""
