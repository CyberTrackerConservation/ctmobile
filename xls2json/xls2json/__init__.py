import logging

import azure.functions as func
from tempfile import TemporaryDirectory
import os.path
import json

from pyxform import xls2json

def main(req: func.HttpRequest) -> func.HttpResponse:
    logging.info('Python HTTP trigger function processed a request.')

    with TemporaryDirectory() as temp_dir_name:
        try:
            with open(
                os.path.join(temp_dir_name, "xlsform.xlsx"), "wb"
            ) as xlsform:
                xlsform.write(req.get_body())
                convert_status = []
                json_dict = xls2json.parse_file_to_json(str(xlsform.name), warnings=convert_status)
                return func.HttpResponse(json.dumps(json_dict, ensure_ascii=False), status_code=200)

        except Exception as e:
            return func.HttpResponse("Bad request", status_code=400)
