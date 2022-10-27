import os
import time
import uvicorn
import logging
import argparse

from inference import Inference
from paddleocr import PaddleOCR

from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse
from fastapi.exceptions import RequestValidationError

app = FastAPI()

logger = logging.getLogger(__name__)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', action='store_true')
    return parser.parse_args()


args = parse_args()
if args.debug:
    logging.getLogger('Debug Mode').setLevel(logging.DEBUG)

IMAGE_PATH = ""
try:
    if "IMAGE_PATH" in os.environ:
        if os.environ["IMAGE_PATH"] == "undefined" or os.environ["IMAGE_PATH"] == "":
            logger.error("Environment Variable - IMAGE_PATH value is error")
            os._exit(0)
        else:
            IMAGE_PATH = os.environ["IMAGE_PATH"]
            ocr = PaddleOCR(use_angle_cls=False, use_gpu=True, lang="en")  # need to run only once to download and load model into memory
    else:
        logger.error("Environment Variable - IMAGE_PATH value is missing")
        os._exit(0)
except Exception as e:
    logger.error(e)
    os._exit(0)

@app.exception_handler(RequestValidationError)
def request_validation_exception_handler(request: Request, exc: RequestValidationError):
    print(f"引數不對{request.method} {request.url}")
    return JSONResponse({"code": "400", "message": exc.errors()})


@app.get("/inference")
def main():
    try:
        since = time.perf_counter()
        result = Inference.run(ocr, IMAGE_PATH)
        time_elapsed = time.perf_counter() - since
        logger.info(str(round(time_elapsed * 1000, 2)) + "ms")
        logger.info(result)
        return result
    except Exception as e:
        result = {"code": "400", "message": str(e)}
        logger.error(result)
        return result


if __name__ == "__main__":
    Inference.run(ocr, IMAGE_PATH)
    uvicorn.run("api:app", host="0.0.0.0", port=8000, reload=args.debug, debug=args.debug)