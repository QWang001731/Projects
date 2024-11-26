import sys,zmq,random,os,time,threading
from utils import serialize,deserialize
from multiprocessing import Pool,Lock
from collections import defaultdict
num_worker=sys.argv[1]
dispatcher_url=sys.argv[2]
results=[]

def execute_task(task_id,fn_payload,param_payload):
    print(f"Process {os.getpid()} started at {time.time()}")
    try:
        func=deserialize(fn_payload)
        params=deserialize(param_payload)[0][0]
        result=func(params)
        print(f"Process {os.getpid()} finished at {time.time()}")
        return {"task_id":task_id,"status":"COMPLETED","result":serialize(result)}
    except Exception as e:
        print(f"Process {os.getpid()} encountered error {str(e)} while executing {task_id}")
        return {"task_id":task_id,"status":"FAILED","result":f"error:{str(e)}"}

def send_heartbeat(heartbeat_socket,worker_id):
    cnt=0
    visited=defaultdict(bool)
    ack=False
    while True:
        for result in results:
            if result.ready() and not visited[result]:
                r=(worker_id,serialize(result.get()))
                heartbeat_socket.send_string(serialize(r))
                visited[result]=True
                rep=heartbeat_socket.recv_string()
                print(rep)
        
        dict={"heartbeat":worker_id}
        heartbeat_socket.send_string(serialize(dict))
        rep=heartbeat_socket.recv_string()
        cnt+=1
        if rep=="heartbeat_received" and cnt%20==0:
            print(rep)
        if rep=="Seems you are not working normally.\
Please check and register again before sending requests" and not ack:
            print(rep)
            ack=True
            break
        time.sleep(.1)

context=zmq.Context()
if __name__=='__main__':
    num_worker=int(sys.argv[1])
    dispatcher_url=sys.argv[2]
    heartbeat_socket=context.socket(zmq.REQ)
    worker=context.socket(zmq.DEALER)
    port=6666
    worker_id=f"worker-{random.randint(1,1000000)}"
    heartbeat_socket.connect(f"tcp://127.0.0.1:{port+1}")
    heartbeat_thread=threading.Thread(target=send_heartbeat,args=(heartbeat_socket,worker_id,))
    heartbeat_thread.start()
    worker.setsockopt_string(zmq.IDENTITY,worker_id)
    worker.connect(f"{dispatcher_url}:{port}")
    print(f"worker {worker_id} connected and ready to work.")
    with Pool(num_worker) as pool:
        while True:
            message=worker.recv_multipart()
            task=message[0].decode()
            task=deserialize(task)
            task_id=task['task_id']
            fn_payload,args_payload=task['function']
            result=pool.apply_async(execute_task,args=(task_id,fn_payload,args_payload))
            results.append(result)
