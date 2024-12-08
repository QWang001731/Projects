import redis
from utils import serialize, deserialize, RegisterFn,RegisterFnREP,ExecuteFnReq,ExecuteFnRep
import uuid
import json
from multiprocessing import Pool
from multiprocessing import Process
from fastapi import FastAPI
from fastapi.responses import JSONResponse
r=redis.Redis(host='localhost',port=6379,db=0)
app=FastAPI()
@app.get("/result/{task_id}")
async def read_result(task_id:str):
    try:
        obj=r.get(task_id).decode()
        json_obj=json.loads(obj)
        status=json_obj["status"]
        result=json_obj["result"]
        print(status,result)
        return JSONResponse(content={"task_id":task_id,"status":status,"result":result},status_code=200)
    except Exception as e:
        return JSONResponse(content={"error:":{e}},status_code=500)

@app.get("/status/{task_id}")
async def read_status(task_id:str):
    try:
        obj=r.get(task_id).decode()
        json_obj=json.loads(obj)
        status=json_obj["status"]
        return JSONResponse(content={"task_id":task_id,"status":status},status_code=200)
    except Exception as e:
        return JSONResponse(content={"error":{e}},status_code=500)

@app.post("/register_function")
async def register(reg:RegisterFn):
    uid=uuid.uuid4()
    name=reg.name
    payload=reg.payload
    try:
        _=deserialize(payload)
        data={"name":name,"payload":payload}
        json_obj=json.dumps(data)
        r.set(str(uid),json_obj)
        return JSONResponse(content={"function_id":str(uid)},status_code=200)
    
    except Exception as e:
        return JSONResponse(content={"error":str(e)},status_code=500)

@app.post("/execute_function")
async def compute(obj:ExecuteFnReq):
    try:
        uid=obj.function_id
        json_obj=r.get(str(uid))
        func_obj=json.loads(json_obj)
        func=func_obj["payload"]
        task_uuid=str(uuid.uuid4())
        args=obj.payload
        data={"fn_payload":func,"arg_payload":args,
            "result":"unknown","status":"QUEUED"}
        json_obj=json.dumps(data)
        r.set(task_uuid,json_obj)
        r.publish('tasks',task_uuid)
        return JSONResponse(content={"task_id":task_uuid},status_code=200)
    except Exception as e:
        return JSONResponse(content={"error":str(e)},status_code=500)