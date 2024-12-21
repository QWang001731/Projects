#include <zmq.hpp>
#include<nlohmann/json.hpp>
#include<iostream>
#include<random>
using namespace std;
using json=nlohmann::json;
int main(int argc ,char** argv){
    zmq::context_t context{1};
    zmq::socket_t socket(context, zmq::socket_type::req);
    socket.connect("tcp://localhost:5555");
    int num_of_message = atoi(argv[1]);
    int cnt=0;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<>dist(0,3);
    uniform_int_distribution<>dist1(1,6);
    for(int i=0;i<num_of_message;i++){
        int r=dist(gen);
        json task;
        if(r==0||r==1){
            task["command"]="ADD";
            task["timestamp"]=dist1(gen);
            task["body"]=dist1(gen);
            
        }else if(r==2){
            task["command"]="CONTAINS";
            task["timestamp"]=dist1(gen);
        }else{
            task["command"]="REMOVE";
            task["timestamp"]=dist1(gen);
        }
            string task_str=task.dump();
            zmq::message_t task_bin(task_str.begin(),task_str.end());
            socket.send(task_bin,zmq::send_flags::none);
            zmq::message_t rep;
            zmq::recv_result_t res=socket.recv(rep,zmq::recv_flags::none);
            std::string message(static_cast<char*>(rep.data()), rep.size());
            cout<<message<<endl;
            cnt++;
    }
    json finish;
    finish["command"]="DONE";
    string finish_str=finish.dump();
    zmq::message_t finish_bin(finish_str.begin(), finish_str.end());
    socket.send(finish_bin);
    zmq::message_t rep;
    zmq::recv_result_t res=socket.recv(rep,zmq::recv_flags::none);
    cnt++;
    cout<<cnt<<" "<<rep.to_string()<<" received"<<endl;

}