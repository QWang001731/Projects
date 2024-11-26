import sys,zmq,time,json,os,random,threading
from utils import serialize,deserialize
from multiprocessing import Pool,Lock
from collections import defaultdict

context=zmq.Context()
results=[]
def execute_task(task_id,fn_payload,param_payload,url):
    print(f"Process {os.getpid()} started at {time.time()}")
    try:
        func=deserialize(fn_payload)
        params=deserialize(param_payload)[0][0]
        result=func(params)
        rep={"result":(task_id,"COMPLETED",serialize(result))}
        print(f"Process {os.getpid()} finished at {time.time()}")
        return rep
    
    except Exception as e:
        rep={"result":(task_id,"FAILED",f"error:{str(e)}")}
        print(f"Process {os.getpid()} encountered error {str(e)} while executing {task_id}")
        return rep
    

def return_result(socket):
    global results
    sent=defaultdict(bool)
    while True:
        for result in results:
            if result.ready() and not sent[result]:
                r=result.get()
                socket.send_string(serialize(r))
                rep=socket.recv_string()
                sent[result]=True
                print(rep)
        time.sleep(.05)
        
if __name__=='__main__':
    num_worker=int(sys.argv[1])
    dispatcher_url=sys.argv[2]
    socket=context.socket(zmq.REQ)
    lock=Lock()
    reg_socket=context.socket(zmq.REQ)
    port=6666
    result_failure_notified=False
    registration_failure_notified=False
    socket.connect(f"{dispatcher_url}:{port}")
    reg_socket.connect(f"{dispatcher_url}:{int(port)+1}")
    worker_id=random.randint(1,1000000)
    registration_info=f"worker-{worker_id}"
    reg_socket.send_string(serialize(registration_info))
    reply=reg_socket.recv_string()
    print(reply)

    cnt=0
    result_thread=threading.Thread(target=return_result,args=(reg_socket,))
    result_thread.start()

    time.sleep(.1)
    
    with Pool(processes=num_worker) as pool:
        while True:
            socket.send_string(serialize({"request":registration_info}))
            reply=socket.recv_string()
            task=deserialize(reply)
            if task!="There is no task currently" and task!="Seems you are not working normally.\
Please check and register again before sending requests":
                result=pool.apply_async(execute_task,
                args=(task['task_id'],task['function'][0],task['function'][1],f"{dispatcher_url}:{port}",))
                results.append(result)
                
            elif task=="There is no task currently":
                if cnt%200==0:
                    print(task)
                cnt+=1
            else:
                if not registration_failure_notified:
                    print(task)
                    registration_failure_notified=True