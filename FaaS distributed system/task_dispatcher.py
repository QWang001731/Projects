import json,os,time,zmq,threading,redis,argparse
from multiprocessing import Pool
from utils import serialize,deserialize
from collections import defaultdict
r=redis.Redis(host='localhost',port=6379,db=0)
lock=threading.Lock()
registered_workers=[]
available_workers=[]
failed_tasks=[]
failed_workers=[]
registeration_complete=0
task_tracker=dict()
context=zmq.Context()

def execute_task(task_id,fn_payload,param_payload):
    print(f"Process {os.getpid()} started at {time.time()}")
    try:
        func=deserialize(fn_payload)
        params=deserialize(param_payload)[0][0]
        json_obj=json.dumps({"task_id":task_id, "status":"RUNNING", "result":"unknown"})
        r.set(task_id,json_obj)
        result=func(params)
        json_obj=json.dumps({"task_id":task_id, "status":"COMPLETED", "result":serialize(result)})
        r.set(task_id,json_obj)
        print(f"Process {os.getpid()} finished at {time.time()}")
        return
    
    except Exception as e:
        json_obj=json.dumps({"task_id":task_id, "status":"FAILED", "result":f"error:{str(e)}"})
        r.set(task_id,json_obj)
        print(f"Process {os.getpid()} encountered error {str(e)} at {time.time()}")
        return 


def heartbeat_and_result_rcv_for_push_worker(lock,heart_beat,port):
    heartbeat_socket=context.socket(zmq.REP)
    heartbeat_socket.bind(f"tcp://127.0.0.1:{port}")
    heartbeat_socket.setsockopt(zmq.RCVTIMEO,200)
    except_cnt=0
    run=1
    while run:
        worker_to_remove=None
        for worker in heart_beat:
            if time.time()-heart_beat[worker]>1 and worker in available_workers:
                worker_to_remove=worker
                with lock:
                    available_workers.remove(worker_to_remove)
                    registered_workers.remove(worker_to_remove)
                    failed_workers.append(worker_to_remove)
                print(f"{worker} is down and removed")

        for worker in failed_workers:
            if worker in heart_beat:
                heart_beat.pop(worker)

        try:
            msg=heartbeat_socket.recv_string()
            msg=deserialize(msg)
            if "heartbeat" in msg:
                worker_id=msg["heartbeat"]
                if worker_id not in failed_workers:
                    heart_beat[worker_id]=time.time()
                    with lock:
                        heartbeat_socket.send_string("heartbeat_received")
                        if worker_id not in registered_workers:
                            registered_workers.append(worker_id)
                else:
                    with lock:
                        heartbeat_socket.send_string("Seems you are not working normally.\
Please check and register again before sending requests")
                        if worker_id in registered_workers:
                            registered_workers.remove(worker_id)

            elif type(msg) is tuple:
                worker_id=msg[0]
                result=deserialize(msg[1])
                task_id=result["task_id"]
                if worker_id in available_workers:
                    print(result)
                    heartbeat_socket.send_string("result recieved")
                    r.set(task_id,json.dumps(result))
                else:
                    heartbeat_socket.send_string(f"{task_id} result timeout and will not be accepted.")
                
                with lock:
                    if task_id in task_tracker:
                        task_tracker.pop(task_id)

        except zmq.Again:
            if registeration_complete:
                except_cnt+=1
            if except_cnt>=10:
                run=0
                print("all workers are down.")
            else:
                continue

def registeration_and_result_rcv_for_pull_worker(socket,lock):
    while True:
        msg=socket.recv_string()
        msg=deserialize(msg)
        if  type(msg) is str:
            worker_id=msg
            socket.send_string(f"{worker_id} you are registered")
            if worker_id not in registered_workers:
                registered_workers.append(worker_id)

        else:
            result=msg["result"]
            task_id=result[0]
            if task_id not in failed_tasks:
                dict={"task_id":result[0],"status":result[1],"result":result[2]}
                json_obj=json.dumps(dict)
                r.set(task_id,json_obj)
                print(f"result of {task_id} stored to redis")
                socket.send_string(f"result of {task_id} received")
                with lock:
                    if task_id in task_tracker:
                        task_tracker.pop(task_id)
                        #print(f"{task_id} poped")
            else:
                socket.send_string(f"result of task {task_id} timeout and will not be accepted")

if __name__=='__main__':
    parser=argparse.ArgumentParser(description="task_dispatcher")
    parser.add_argument("-m",choices=["local","pull","push"])
    parser.add_argument("-p",type=int)
    parser.add_argument("-w",type=int)
    args=parser.parse_args()
    pubsub=r.pubsub()
    pubsub.subscribe('tasks')
    if args.m=="local":
        num_workers=args.w
        print(num_workers," workers are working...")
        with Pool(num_workers) as pool:
            while True:
                message=pubsub.get_message()
                if message and message['type']=='message':
                    task_id=message['data'].decode()
                    json_obj=json.loads(r.get(task_id).decode())
                    fn_payload=json_obj['fn_payload']
                    args_payload=json_obj['arg_payload']
                    pool.apply_async(execute_task, 
                            args=(task_id,fn_payload,args_payload,))
                    
    elif args.m=='pull':
        port=args.p
        socket=context.socket(zmq.REP)
        socket.bind(f"tcp://127.0.0.1:{port}")
        register_socket=context.socket(zmq.REP)
        register_socket.bind(f"tcp://127.0.0.1:{int(port)+1}")
        reg_thread=threading.Thread(target=registeration_and_result_rcv_for_pull_worker,
                                    args=(register_socket,lock))
        reg_thread.start()
        #wait 10 seconds for worker registeration
        time.sleep(10)
        print(f"registration complete, program runs with workers {registered_workers}")
        for worker in registered_workers:
            available_workers.append(worker)

        while True:
            continue_to_check=False
            for task in task_tracker:
                if time.time()-task_tracker[task][0]>2:
                    dict=json.loads(r.get(task))
                    dict['status']="FAILED"
                    dict['result']="Timeout-error"
                    r.set(task,json.dumps(dict))
                    continue_to_check=True
                    worker_to_remove=task_tracker[task][1]
                    with lock:
                        if worker_to_remove in available_workers:
                            available_workers.remove(worker_to_remove)
                        if worker_to_remove in registered_workers:
                            registered_workers.remove(worker_to_remove)
                        if task not in failed_tasks:
                            failed_tasks.append(task)
                    #print(f"removed {worker_to_remove}, time {time.time()-task_tracker[task][0]}")
                    print(f"WorkerFailure Exception: {worker_to_remove} failed to execute task{task}.")

            for task in failed_tasks:
                if task in task_tracker:
                    task_tracker.pop(task)
            if continue_to_check:
                continue
            req=socket.recv_string()
            req=deserialize(req)
            if req["request"] not in available_workers:
                socket.send_string(serialize("Seems you are not working normally.\
Please check and register again before sending requests"))
                continue
            else:
                start=time.time()
                while True:
                    message=pubsub.get_message()
                    if message and message['type']=='message':
                        task_id=message['data'].decode()
                        json_obj=json.loads(r.get(task_id).decode())
                        fn_payload=json_obj['fn_payload']
                        args_payload=json_obj['arg_payload']
                        task={"task_id":task_id, 
                            "function":(fn_payload,args_payload)}
                        socket.send_string(serialize(task))
                        task_tracker[task_id]=(time.time(),req['request'])
                        json_obj['status']='RUNNING'
                        r.set(task_id,json.dumps(json_obj))
                        break
                    elif time.time()-start>.1:
                        socket.send_string(serialize("There is no task currently"))
                        break
                    else:
                        continue
              
    elif args.m=='push':
        heart_beat=defaultdict(float)
        socket=context.socket(zmq.ROUTER)
        port=args.p
        socket.bind(f"tcp://127.0.0.1:{port}")
        heartbeat_thread=threading.Thread(target=heartbeat_and_result_rcv_for_push_worker,
                                          args=(lock,heart_beat,int(port)+1))
        heartbeat_thread.start()
        #wait 10 seconds for worker registeration
        time.sleep(10)
        for worker in registered_workers:
            available_workers.append(worker)
        print(f"Registeration complete.\nWith available workers {available_workers},program starts running...")
        registeration_complete=1
        task_num=0
        while True:
            continue_to_check=False
            for task in task_tracker:
                if time.time()-task_tracker[task][0]>2:
                    dict={"task_id":task,"status":"FAILED","result":"time out error"}
                    json_obj=json.dumps(dict)
                    r.set(task,json_obj)
                    continue_to_check=True
                    worker_to_remove=task_tracker[task][1]
                    with lock:
                        if worker_to_remove in available_workers:
                            available_workers.remove(worker_to_remove)
                        if worker_to_remove in registered_workers:
                            registered_workers.remove(worker_to_remove)
                        if worker_to_remove not in failed_workers:
                            failed_workers.append(worker_to_remove)
                        if task not in failed_tasks:
                            failed_tasks.append(task)
                    print(f"WorkerFailure Exception: {worker_to_remove} failed to execute task{task}.")
            
            for task in failed_tasks:
                if task in task_tracker:
                    task_tracker.pop(task)
            
            if continue_to_check:
                continue
            
            message=pubsub.get_message()           
            if message and message['type']=='message':
                task_id=message['data'].decode()
                dic=json.loads(r.get(task_id))
                fn_payload=dic['fn_payload']
                args_payload=dic['arg_payload']                        
                task={"task_id":task_id, 
                    "function":(fn_payload,args_payload)}
                task=serialize(task)
                task_num+=1
                while not available_workers:
                    print("No workers avaialbe, waiting...")
                    time.sleep(.3)
                selected_idx=task_num%len(available_workers)
                worker_selected=available_workers[selected_idx]
                socket.send_multipart([worker_selected.encode(),task.encode()])
                task_tracker[task_id]=(time.time(),worker_selected)
                dic['status']='RUNNING'
                r.set(task_id,json.dumps(dic))
                print(f"work sent to {worker_selected}")