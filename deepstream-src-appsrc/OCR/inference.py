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
                if ("DOT" or "DDT" or "D0T" or "OOT" or "ODT" or "O0T" or "0OT" or "0DT" or "00T") in ocr_result:
                    ocr_result = str.strip(re.sub("[a-zA-Z#-.]", "", ocr_result))
                    if 6 <= len(ocr_result) <= 8:
                        return ocr_result
                    else:
                        return ""
        return ""
