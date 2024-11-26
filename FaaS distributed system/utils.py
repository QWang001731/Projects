import codecs
import dill
import uuid
from pydantic import BaseModel

def serialize(obj) -> str:
    return codecs.encode(dill.dumps(obj), "base64").decode()


def deserialize(obj: str):
    return dill.loads(codecs.decode(obj.encode(), "base64"))


def execute_fn(task_id:uuid.UUID,ser_fn:str,ser_params:str):
    fn=deserialize(ser_fn)
    params=deserialize(ser_params)
    result=fn(params)
    return serialize(result)



class RegisterFn(BaseModel):
    name:str
    payload:str

class RegisterFnREP(BaseModel):
    function_id:uuid.UUID


class ExecuteFnReq(BaseModel):
    function_id:str
    payload:str


class ExecuteFnRep(BaseModel):
    task_id:str

