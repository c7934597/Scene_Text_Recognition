import re
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
logger = logging.getLogger(__name__)
class Inference:
    def run(ocr, image_path) -> str:
        result = ocr.ocr(image_path, cls = False)
        for line in result:
            if "USDOT" in line[1][0] or "US DOT" in line[1][0] or "U.S.DOT" in line[1][0] or "U.S. DOT" in line[1][0]:
                ocr_result = str.strip(re.sub("[a-zA-Z#.]", "", line[1][0]))
                if 6 <= len(ocr_result) <= 8:
                    return ocr_result
                else:
                    return ""
        return ""
