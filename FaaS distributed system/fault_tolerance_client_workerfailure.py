import requests,time
from utils import serialize, deserialize


base_url = "http://127.0.0.1:8000/"
valid_statuses = ["QUEUED", "RUNNING", "COMPLETED", "FAILED"]

def double(x):
    import time
    time.sleep(.1)
    return x * 2


def test_fn_registration():
    # Using a real serialized function
    serialized_fn = serialize(double)
    resp = requests.post(base_url + "register_function",
                         json={"name": "double",
                               "payload": serialized_fn})

    assert resp.status_code in [200, 201]
    assert "function_id" in resp.json()

def test_execute_fn():
    resp = requests.post(base_url + "register_function",
                         json={"name": "hello",
                               "payload": serialize(double)})
    fn_info = resp.json()
    assert "function_id" in fn_info
    resp = requests.post(base_url + "execute_function",
                         json={"function_id": fn_info['function_id'],
                               "payload": serialize(((2,), {}))})

    #print(resp)
    assert resp.status_code == 200 or resp.status_code == 201
    assert "task_id" in resp.json()
    task_id = resp.json()["task_id"]
    resp = requests.get(f"{base_url}status/{task_id}")
    #print(resp.json())
    assert resp.status_code == 200
    assert resp.json()["task_id"] == task_id
    assert resp.json()["status"] in valid_statuses
    return task_id
    


if __name__=="__main__":
    task_ids=[]
    test_fn_registration()
    for _ in range(10):
        task_id=test_execute_fn()
        task_ids.append(task_id)
    
    time.sleep(15)

    for task_id in task_ids:
        resp = requests.get(f"{base_url}result/{task_id}")
        print(resp.json())
    
    