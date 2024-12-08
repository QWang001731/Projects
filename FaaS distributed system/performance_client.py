from utils import serialize,deserialize
import json,sys,requests,redis,logging,time,random,zmq,random,argparse
from typing import Optional
import logging
from collections import defaultdict

base_url = "http://127.0.0.1:8000/"
valid_statuses = ["QUEUED", "RUNNING", "COMPLETED", "FAILED"]
r=redis.Redis(host='localhost',port=6379,db=0)
tasks=[]
def double(x):
    import time
    time.sleep(.1)
    return x * 2


def test_latency(number_of_tasks):
    resp = requests.post(base_url + "register_function",
                         json={"name": "double",
                               "payload": serialize(double)})
    fn_info = resp.json()
    number = 10
    complete=defaultdict(bool)
    for _ in range(number_of_tasks):
        resp = requests.post(base_url + "execute_function",
                            json={"function_id": fn_info['function_id'],
                                "payload": serialize(((number,), {}))})
        task_id = resp.json()["task_id"]
        tasks.append(task_id)

        assert resp.status_code in [200, 201]
        assert "task_id" in resp.json()

    task_id = resp.json()["task_id"]
    cnt=0
    while cnt!=number_of_tasks:
        for task_id in tasks:
            if not complete[task_id]:
                resp = requests.get(f"{base_url}result/{task_id}")
                assert resp.status_code == 200
                assert resp.json()["task_id"] == task_id
                #print(resp.json()['status'])
                if resp.json()['status']=="COMPLETED":
                    complete[task_id]=True
                    cnt+=1
                    #logging.warning(f"Task is now in {resp.json()['status']}")
                    s_result = resp.json()
                    #logging.warning(s_result)
                    result = s_result['result']
                    result = deserialize(result)
                    assert result == number*2

if __name__=='__main__':
    parser=argparse.ArgumentParser(description="client")
    parser.add_argument("-t",type=int)
    args=parser.parse_args()
    num_task=args.t
    t1=time.time()
    test_latency(number_of_tasks=num_task)
    t2=time.time()
    print(f"execution latency: {t2-t1}")
