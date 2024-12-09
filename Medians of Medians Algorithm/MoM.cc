#include<vector>
#include <iostream>
#include<algorithm>
#include<cmath>
#include<cassert>
using namespace std;
int medianOfMedian(vector<int>&v, int k,int left, int right){
    int n =right - left + 1;
    int m=n/5;
    if (5 * m < n) 
        m++;
    vector<vector<optional<int>>>matrix(m, vector<optional<int>>(5,nullopt));
    vector<int>median(m);
    for(int i=left;i<=right;i++){
        int row = (i-left)/5;
        int col = (i-left)%5;
        matrix[row][col]=v[i];
        }

    for(int i=0;i<m;i++)
        median[i]=median_element(matrix[i]);
        
    int med=median_element(median);
    int medidx=get_idx(v,med);
    
    int t = v[medidx];
    v[medidx]=v[right];
    v[right]=t;
    
    int i=left-1;
    int x = v[right];
    for(int j=left;j<right;j++){
        if(v[j]<=x){
            i++;
            int t = v[i];
            v[i]=v[j];
            v[j]=t;
            }
        }
    int t1 = v[i+1];
    v[i+1]=v[right];
    v[right]=t1;

    int h=n-(i-left+1);
    if(k==h) 
        return v[i+1];
    else if (k>h) 
        return medianOfMedian(v,k-h,left,i);
    else 
        return medianOfMedian(v,k,i+2,right);
}

int main(){
    vector<int>v{1,9,1,7,8,6,4,3,11,23,19,22,100,20000};
    cout<<medianOfMedian(v,6,0,13);
}
