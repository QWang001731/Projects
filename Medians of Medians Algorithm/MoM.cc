#include<vector>
#include <iostream>
#include<algorithm>
#include<cmath>
#include<cassert>
using namespace std;

template<class T>
T Median(vector<T>v){
    sort(v.begin(),v.end());
    return v[v.size()/2];
}
template<class T>
T MoM(vector<T>v, size_t i){
    assert(i<=v.size());
    if (v.size()==1) return v[0];
    int num_groups=ceil(v.size()/5.);
    vector<vector<T>> sub_group(num_groups, vector<T>(5,0));
    int last_group_num = v.size()-5*(num_groups-1);
    sub_group[num_groups-1].resize(last_group_num,0);

    for(int i1=0;i1<num_groups-1;i1++){
        int start = i1*5; 
        for(int i2=0;i2<5;i2++)
            sub_group[i1][i2]=v[start+i2];
    }

    int start = 5*(num_groups-1);
    for(int i1=0;i1<last_group_num;i1++)
        sub_group[num_groups-1][i1]=v[start+i1];
    
    vector<T> medians(num_groups, 0);
    for(int ii=0;ii<num_groups;ii++)
        medians[ii] = Median<T>(sub_group[ii]);
    
    T m = MoM<T>(medians, medians.size()/2+1);
    vector<T>v1,v2;
    for(int j=0;j<v.size();j++){
        if(v[j]<m) 
            v1.push_back(v[j]);
        else if(v[j]>m) 
            v2.push_back(v[j]);
    }

    if (v1.size()==i-1) 
        return m;
    else if(v1.size()>i-1) 
        return MoM<T>(v1, i);
    else 
        return MoM<T>(v2, i-(1+v1.size()));
}

int main(){
    vector<int>v{1,9,1,7,8,6,4,3,11,23,19,22,100,20000};
    cout<<MoM(v,6);
}
