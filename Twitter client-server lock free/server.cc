#include<zmq.hpp>
#include<iostream>
#include<nlohmann/json.hpp>
#include<fstream>
#include<atomic>
#include<condition_variable>
#include<thread>
#include<feeds.hpp>
#include<task.hpp>
#include<omp.h>

using namespace std;
using json=nlohmann::json;

void process_task(Feed* f, Task*t, int thread_id, ofstream &log)
{
    string command=t->task["command"];
    if(command=="ADD")
    {
        string timestamp=to_string(t->task["timestamp"]);
        f->Add(t->task,thread_id);
        string rep=timestamp+" post added to feed by thread " + to_string(thread_id);
        //cout<<rep<<endl;
        log<<rep<<endl;
    }
    else if (command=="REMOVE")
    {
        int timestamp=t->task["timestamp"];
        bool res=f->Remove(timestamp,thread_id);
        if(res)
        {
            string rep="post of timestamp " + to_string(timestamp) + " removed by thread" + to_string(thread_id);
            //cout<<rep<<endl;
            log<<rep<<endl;
        }
        else
        {
            string rep="post of timestamp " + to_string(timestamp) + " is not in feed by thread" + to_string(thread_id);
            //cout<<rep<<endl;
            log<<rep<<endl;

        }
        
    }else if(command=="CONTAINS")
    {
        int timestamp=t->task["timestamp"];
        bool res=f->Contains(timestamp,thread_id);
        if(res)
        {
            string rep="feed contains post of timestamp " + to_string(timestamp) + " by thread" + to_string(thread_id);
            //cout<<rep<<endl;
            log<<rep<<endl;
        }
        else
        {
            string rep="feed doesn't contain post of timestamp " + to_string(timestamp) + " by thread" + to_string(thread_id);
            //cout<<rep<<endl;
            log<<rep<<endl;
        }
    }
}


void process_task(Lk_free_feed* f, Task*t, int thread_id, ofstream &log)
{
    string command=t->task["command"];
    if(command=="ADD")
    {
        string timestamp=to_string(t->task["timestamp"]);
        f->Add(t->task,thread_id);
        string rep=timestamp+" post added to feed by thread " + to_string(thread_id);
        //cout<<rep<<endl;
        log<<rep<<endl;
    }
    else if (command=="REMOVE")
    {
        int timestamp=t->task["timestamp"];
        bool res=f->Remove(timestamp,thread_id);
        if(res)
        {
            string rep="post of timestamp " + to_string(timestamp) + " removed by thread" + to_string(thread_id);
            //cout<<rep<<endl;
            log<<rep<<endl;
        }
        else
        {
            string rep="post of timestamp " + to_string(timestamp) + " is not in feed by thread" + to_string(thread_id);
            //cout<<rep<<endl;
            log<<rep<<endl;

        }
        
    }else if(command=="CONTAINS")
    {
        int timestamp=t->task["timestamp"];
        bool res=f->Contains(timestamp,thread_id);
        if(res)
        {
            string rep="feed contains post of timestamp " + to_string(timestamp) + " by thread" + to_string(thread_id);
            log<<rep<<endl;
        }
        else
        {
            string rep="feed doesn't contain post of timestamp " + to_string(timestamp) + " by thread" + to_string(thread_id);
            log<<rep<<endl;
        }
    }
}

void consumer(Feed*f,Task_queue *tq,int thread_id)
{
    string log_name="log_thread_" + to_string(thread_id) + ".txt";
    ofstream log(log_name);
    while(true)
    {   
        Task *expected=nullptr;
        if(tq->head.compare_exchange_strong(expected, nullptr))
        {
            string msg="empty task queue,thread"+to_string(thread_id) + " sleep...\n";
            //cout<<msg;
            std::unique_lock<mutex>lock(mu);
            has_work=false;
            cv.wait(lock,[]{ return has_work || finish;});
            if(finish)
            {   
                cout<<"thread"<<thread_id<<" finish"<<endl;
                break;
            }
        }
        else
        {
            Task * t=tq->dequeue();
            if(t==nullptr)
            {
                string msg="after dequeue thread"+to_string(thread_id) + " no work, sleep...\n";
                //cout<<msg;
                std::unique_lock<mutex>lock{mu};
                has_work=false;
                cv.wait(lock, []{return has_work || finish;});
                if(finish)
                {
                    cout<<"thread"<<thread_id<<" finish"<<endl;
                    break;
                }
            }
            else
            {
                string comm=(t->task["command"]);
                string msg="thread"+to_string(thread_id)+"going to process task " + comm +string("\n");
                //cout<<msg;
                process_task(f,t,thread_id,log);
            }
        }
    }
}

void consumer_lkfree(Lk_free_feed*f,Task_queue *tq,int thread_id)
{
    string log_name="log_thread_" + to_string(thread_id) + ".txt";
    ofstream log(log_name);
    while(true)
    {   
        Task *expected=nullptr;
        if(tq->head.compare_exchange_strong(expected, nullptr))
        {
            string msg="empty task queue,thread"+to_string(thread_id) + " sleep...\n";
            //cout<<msg;
            std::unique_lock<mutex>lock(mu);
            has_work=false;
            cv.wait(lock,[]{ return has_work || finish;});
            if(finish)
            {   
                cout<<"thread"<<thread_id<<" finish"<<endl;
                break;
            }
        }
        else
        {
            Task * t=tq->dequeue();
            if(t==nullptr)
            {
                string msg="after dequeue thread"+to_string(thread_id) + " no work, sleep...\n";
                //cout<<msg;
                std::unique_lock<mutex>lock{mu};
                has_work=false;
                cv.wait(lock, []{return has_work || finish;});
                if(finish)
                {
                    cout<<"thread"<<thread_id<<" finish"<<endl;
                    break;
                }
            }
            else
            {
                string comm=(t->task["command"]);
                string msg="thread"+to_string(thread_id)+"going to process task " + comm +string("\n");
                //cout<<msg;
                process_task(f,t,thread_id,log);
            }
        }
    }
}

int main(int argc ,char** argv)
{
    int n = atoi(argv[1]);
    int lk_free=atoi(argv[2]);
    zmq::context_t context{1};
    zmq::socket_t  socket{context,zmq::socket_type::rep};
    socket.bind("tcp://localhost:5555");
    zmq::message_t task;
    string task_str;
    zmq::recv_result_t res;
    json task_recv;
    Task_queue *tq=new Task_queue();
    Feed *f,*f1;
    Lk_free_feed *lkf,*lkf1;
    if(lk_free==1)
    {
        lkf=new Lk_free_feed();
        lkf1=new Lk_free_feed();
    }
    else
    {
        f=new Feed();
        f1=new Feed();
    }
    
    ofstream log("log.txt");
    vector<thread> threads;

    for(int i=0;i<n;i++)
    {
        if(lk_free==1)
        threads.emplace_back(consumer_lkfree,lkf,tq,i);
        else
        threads.emplace_back(consumer,f,tq,i);
    }
    double t1;
    bool captured{false};
    while(true)
    {
        res=socket.recv(task, zmq::recv_flags::none);
        if(!captured)
        {
            t1=omp_get_wtime();
            captured=true;
        }
        task_str=task.to_string();
        task_recv=json::parse(task_str);
        log<<task_recv<<endl;
        socket.send(zmq::buffer("ack"));
        if(task_recv["command"]=="DONE")
        {
            finish=true;
            cv.notify_all();
            break;
        }

        Task*t=new Task(task_recv);
        tq->enqueue(t);
    }

    for(int i=0;i<n;i++)
        threads[i].join();
    
    double t2=omp_get_wtime();
    ofstream result("results.txt");
    if(lk_free==1)
        lkf->print(result);
    else
        f->print(result);

    cout<<"main thread finish"<<endl;
    cout<<"time:"<<(t2-t1)<<" s"<<endl;

    /*start test*/

    ifstream file("log.txt");
    string line;

    while(getline(file,line))
    {
    json obj=json::parse(line);
    string command=obj["command"];
    if(command=="ADD")
        if(lk_free==1)
            lkf1->Add(obj,0);
        else
            f1->Add(obj,0);
    else if (command=="REMOVE")
        if(lk_free==1)
            lkf1->Remove(obj["timestamp"],0);
        else
            f1->Remove(obj["timestamp"],0);
    else if(command=="CONTAINS")
        if(lk_free==1)
            lkf1->Contains(obj["timestamp"],0);
        else
            f1->Contains(obj["timestamp"],0);
    else if(command=="DONE")
        break;
    }

    ofstream expected("expected.txt");
    if(lk_free==1)
        lkf1->print(expected);
    else
        f1->print(expected);

    ifstream exp("expected.txt"), got("results.txt");
    string exp_1,got_1;

    while(getline(exp,exp_1) && getline(got,got_1))
    {
        if(exp_1!=got_1)
            {
            cerr<<"\033[31mtest failed\033[0m";
            cout<<"expected:"<<exp_1<<" got:"<<got_1<<endl;
            return 0;
            }
    }
    if(lk_free==1)
    cout<<"\033[1;32m lock_free test passed\033[0m";
    else
    cout<<"\033[1;32m coarse_grain test passed\033[0m";

    delete tq;
    delete f;
}
