#include<nlohmann/json.hpp>
#include<iostream>
#include<fstream>
#include <mutex>
#include <cstdint>
using json=nlohmann::json;
using namespace std;
struct Lkfree_post;
struct post
{
    int timestamp;
    int body;
    post *next;
    post()
    {
        timestamp=0;
        body=0;
        next=nullptr;
    }

    post(json&j)
    {
        if(!j.contains("timestamp") || !j.contains("body"))
        {
            throw invalid_argument("invalid JSON object");
        }
        timestamp=j["timestamp"];
        body=j["body"];
        next=nullptr;
    }
};



struct AtomicMarkableReference
{
    atomic<Lkfree_post*>next;
    AtomicMarkableReference()
    {
        next.store(nullptr);
    }

    AtomicMarkableReference(Lkfree_post*p1)
    {
        next.store(p1);
    }

    bool compare_and_set(Lkfree_post* expectedP, Lkfree_post* newP, bool expected_mark, bool newMark)
    {
        uintptr_t combined_expected=reinterpret_cast<uintptr_t>(expectedP) | expected_mark;
        uintptr_t combined_new=reinterpret_cast<uintptr_t>(newP) | newMark;
        if(next.compare_exchange_strong(reinterpret_cast<Lkfree_post*&>(combined_expected), reinterpret_cast<Lkfree_post*>(combined_new)))
        {
            return true;
        }
        else
        {   //cout<<"expectedP:"<<reinterpret_cast<uintptr_t>(expectedP)<<" expected mark:"<<expected_mark<<endl;
            //cout<<"got: "<<reinterpret_cast<uintptr_t>(next.load())<<endl;
            return false;
        }

    }
    bool attempt_mark(Lkfree_post* expected, bool newMark)
    {
        uintptr_t flag=1;
        uintptr_t expected_1=reinterpret_cast<uintptr_t>(expected) & (~flag);
        Lkfree_post* expected_ptr=reinterpret_cast<Lkfree_post*>(expected_1);
        uintptr_t new_combined= reinterpret_cast<uintptr_t>(expected) | newMark;

        if(next.compare_exchange_strong(expected, reinterpret_cast<Lkfree_post*>(new_combined)) || next.compare_exchange_strong(expected_ptr, reinterpret_cast<Lkfree_post*>(new_combined)))
        {
            return true;
        }
        else
        {
            return false;

        }

    }

    Lkfree_post* get(bool* ismarked)
    {
        uintptr_t flag=1;
        uintptr_t p1=reinterpret_cast<uintptr_t>(next.load());
        *ismarked=p1 & flag;
        return reinterpret_cast<Lkfree_post*>(p1 & (~flag));
    }
};


struct Lkfree_post
{
    post* postPtr;
    AtomicMarkableReference* amr;

    Lkfree_post(json &j)
    {
        postPtr=new post(j);
        amr=new AtomicMarkableReference();
    }
    Lkfree_post()
    {
        postPtr=nullptr;
        amr=new AtomicMarkableReference();
    }

    ~Lkfree_post()
    {
        delete postPtr;
        delete amr;
    }

};


struct Feed
{
    mutex mu;
    post* head;
    void Add(json& j,int tid);
    bool Contains(int l,int tid);
    bool Remove(int t,int tid);

    Feed()
    {
        head = new post();
    }
    template<class T>
    void print(T &os);
    ~ Feed()
    {
        post*cur=head, *next=nullptr;
        while(cur)
        {
            next=cur;
            delete cur;
            cur=next->next;
        }
    }
};


struct Window{
    Lkfree_post* pre, *curr;
    Window(Lkfree_post*myPre, Lkfree_post*myCurr)
    {
        pre=myPre;
        curr=myCurr;
    }
};
void Feed::Add(json &j,int tid)
{
    lock_guard<mutex> lk(mu);
    int t = j["timestamp"];
    post * cur =head->next, *pre=head;
    while(cur && t>=(cur->timestamp))
      {
        pre=cur;
        cur=cur->next;
      }
    if(cur==nullptr)
    {
        pre->next=new post(j);
    }
    else
    {
        post* p=new post(j);
        p->next=cur;
        pre->next=p;
    }
    return;
}


bool Feed::Remove(int t,int tid)
{
    lock_guard<mutex> lk(mu);
    post * cur=head->next, *pre=head;
    while(cur)
    {
        if((cur->timestamp)==t)
        {
            pre->next=cur->next;
            return true;
        }
        else
        {
            pre=cur;
            cur=cur->next;
        }
    }
    return false;
}

bool Feed::Contains(int t,int tid)
{
    lock_guard<mutex> lk(mu);
    post*cur=head->next;
    while(cur)
    {
        if (cur->timestamp==t)
            return true;
        else    
            cur = cur->next;
    }
    
    return false;
}

template<class T>
void Feed::print(T &os)
{
    if (head->next==nullptr)
    {
        os<<"empty feed.\n";
        return;
    }
    post * cur = head->next;
    while(cur )
    {
        os<<" body:"<<cur->body<<
            " timestamp:"<<cur->timestamp<<endl;
        cur=cur->next;
    }
}


struct Lk_free_feed
{
    Lkfree_post * head;
    Lk_free_feed()
    {
        head=new Lkfree_post();
    }

    Window* find(int timestamp, int tid)
    {
        bool marked{0},false_flag{0},true_flag(1),retry{0};
        Lkfree_post *curr, *pre, *succ;
        
        while(true)
        {   
            retry=false;
            curr=head->amr->next;
            pre=head;
            while(curr)
            {
                succ=curr->amr->get(&marked);
                if(marked)
                {
                    if(curr!=head->amr->next)
                    {
                        if(pre->amr->compare_and_set(curr, succ, false_flag, false_flag))
                        {
                            continue;
                        }
                        else
                        {
                            retry=true;
                            break;
                        }
                    }
                    else
                    {
                        if(head->amr->compare_and_set(curr,succ,false_flag,false_flag))
                        {
                            continue;
                        }

                        else
                        {
                            retry=true;
                            break;
                        }
                    }
                }
                else if ((curr->postPtr->timestamp) < timestamp)
                {
                    pre=curr;
                    curr=succ;
                }
                else
                    break;
            }

            if (retry)
                continue;
            else
                return new Window(pre, curr);
        }
    }


void Add(json&j,int tid)
{
    int timestamp=j["timestamp"];
    while(true)
    {
        Window * w=find(timestamp,tid);
        string pret,curr_str;
        if(w->pre&&w->pre->postPtr)
            pret=to_string(w->pre->postPtr->timestamp);
        else
            pret="null";

        if(w->curr&&w->curr->postPtr)
            curr_str=to_string(w->curr->postPtr->timestamp);
        else
            curr_str="null\n";

        string s="thread"+to_string(tid) + " add" +to_string(timestamp) + " find pre:" +pret +" curr:"+curr_str;
        //cout<<s;
        bool marked{false}, false_flag{false};

        Lkfree_post * p=new Lkfree_post(j);
        Lkfree_post * nptr=nullptr;
        if(p->amr->compare_and_set(nptr, w->curr, false_flag,false_flag) && 
           w->pre->amr->compare_and_set(w->curr,p,false_flag,false_flag))
           {
            //cout<<"result : 1\n";
            return;
           }
            
        else
            {
            //cout<<"result:0 / cas fail \n";
            delete p;
            }
    }
}

bool Remove(int t, int tid)
{
    while(true)
    {
        Window *w=find(t,tid);
        string pret,curr_str;
        if(w->pre&&w->pre->postPtr)
            pret=to_string(w->pre->postPtr->timestamp);
        else
            pret="null";

        if(w->curr&&w->curr->postPtr)
            curr_str=to_string(w->curr->postPtr->timestamp);
        else
            curr_str="null";
        string s="thread"+to_string(tid) + " remove " +to_string(t) + " find pre:" +pret + " curr:"+ curr_str;
        //cout<<s;
        if(w->curr==nullptr)
        {   //cout<<" result: 0 /null \n";
            return false;
        }

        bool flag_true{true},flag_false{false},marked{false};
        Lkfree_post*succ=w->curr->amr->get(&marked);
        if(w->curr->postPtr->timestamp != t)
        {   
            //cout<<" result: 0 / neq\n";
            return false;
        }
        else if (w->curr->amr->attempt_mark(succ, flag_true))
        {   /*
            if (w->pre->amr->compare_and_set(w->curr,succ,flag_false,flag_false))
                cout<<" result: 1 / physical \n";
            else
                cout<<" result: 1 / logical \n";*/
            return true;
        }
        else
        {
            continue;
        }
    }
}

bool Contains(int timestamp,int tid)
{
    bool marked{false};
    Window*w=find(timestamp,tid);
    if(w->curr)
        w->curr->amr->get(&marked);
    return w->curr && ((w->curr->postPtr->timestamp)==timestamp) && !marked;

}

template<class T>
void print(T&os){
    find(2e9,0);
    bool marked;
    Lkfree_post*curr=head->amr->next;
    if(!curr)
    {
        os<<"empty feed"<<endl;
        return;
    }
    while(curr)
    {   
        marked=false;
        curr->amr->get(&marked);
        if(!marked)
            os<<"timestamp: "<<curr->postPtr->timestamp<<" body: "<<curr->postPtr->body<<endl;
        else
            os<<"logical delete "<<"timestamp: "<<curr->postPtr->timestamp<<" body: "<<curr->postPtr->body<<endl;
        
        curr=curr->amr->next;
    }
}

};
