#include<iostream>
#include<nlohmann/json.hpp>
using namespace std;
using json=nlohmann::json;
;
condition_variable cv;
mutex mu;
bool has_work{false};
bool finish {false};

struct Task
{
    json task;
    Task *pre;
    Task *next;
    Task(json t)
    {
        task=t;
        pre=nullptr;
        next=nullptr;
    }

};
struct Task_queue
{
    atomic<Task*>tail, head;
    Task_queue()
    {
        head=nullptr;
        tail=nullptr;
    }

    void enqueue(Task* t)
    {
        while(true)
        {
            Task* expected=tail.load();
            if(tail.compare_exchange_strong(expected, t))
            {
                if(expected!=nullptr)
                {
                    expected->pre=t;
                    t->next=expected;
                    //cout<<"enqueue for non-null tail\n";
                }
                else
                {
                    //cout<<"enqueu for null tail\n";
                    Task* head_expected=nullptr;
                    head.compare_exchange_strong(head_expected, t);

                }
                string msg="enqueue success for " + (t->task).dump() +string("\n");
                //cout<<msg;
                unique_lock<mutex> lock(mu);
                has_work=true;
                lock.unlock();
                cv.notify_one();
                break;
            }
        }
    }

    Task* dequeue()
    {
        while(true)
        {
            Task*expected=nullptr;
            if(head.compare_exchange_strong(expected, nullptr))
            {
                return nullptr;   
            }
            else if(expected->pre!=nullptr)
            {
                if(head.compare_exchange_strong(expected, expected->pre))
                    return expected;
            }
            else
            {
                Task* expected1 = expected;
                if(tail.compare_exchange_strong(expected1, nullptr) && head.compare_exchange_strong(expected,nullptr))
                    return expected;
            }
        }
    }
};