/*
MPCS51044 Final Project: Ray Tracing using C++ multi thread 
	    Student Name : Qi Wang
		CNetID : qiwang001 

Description: Ray tracing calculation can be time-consuming due to the large amount 
of computation time. C++ provides libraries like <future.h> and <thread.h> that 
support a solution of using multiple thread to do computation simultaneously to 
significantly reduce the computation time.

This program uses one and two threads to do the required computation of raytracing of a sphere
, so that it can be seen that the required computation time in 2 thread mode is almost  halved. 
The output file "grid.txt" is the recorded lumination of the sphere. The PLOT.py program takes "grid.txt" 
as input and plots this sphere.

compiler flag: -fopenmp
*/
#include"stdlib.h"
#include"stdio.h"
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include "omp.h"
#include <thread>
#include<iostream>
# define PI 3.1415926
using namespace std;
#include <memory>
#include <future>
#include <utility>

// coordinate struct
template<typename T>
struct coord{T x; T y; T z;};

//T is for double precision or single precision cases
template<typename T>
class ray_tracer
{
public:
    ray_tracer(int m, int N, T r, T wy,T wmax, coord<T>c,coord<T>L,bool d,const char* f): 
    m(m), N(N),r(r),wy(wy),wmax(wmax),c(c),L(L),double_thread(d),filename(f){}
    //this function will create a grid used to PLOT the sphere
    std::pair <shared_ptr<T*>, shared_ptr<T>> grid_maker()
    {
    T* g0 = (T *)calloc(m * m, sizeof(T));
    T ** M = (T**)malloc(sizeof(T*)*m);
    for(int i=0;i<m;i++)
        M[i] = g0 + i * m;
    shared_ptr<T*> UM(M);
    shared_ptr<T>g(g0);
    std::pair<shared_ptr<T*>, shared_ptr<T>> sharePtr;
    sharePtr.first = UM;
    sharePtr.second =g;
    return sharePtr;
    }
////raytracing algorithm that compute the lumination of a sphere under a light source
 shared_ptr<T*> ray_tracing()
{
    T len_s=0, 
    len_n=0,wx, wz,temp,t,b,xx;
    coord<T>I; // intersection of light with window
    coord<T>s,n;
    coord<T> view_light;
    T phi,cosine_theta, sine_theta;
    size_t p,q;
    std::pair <shared_ptr<T*>, shared_ptr<T>> pair_main = grid_maker();
    shared_ptr<T*>grid = pair_main.first;

    //ray tracing algorithm that computes the lumination of a sphere under a light source
    for (int i=0;i<=N;i++)
    {   
        while(true)
        {
        phi = PI* (T)rand()/RAND_MAX;
        cosine_theta = ((T)rand()/RAND_MAX - 0.5)*2;
        sine_theta = sqrt(1-cosine_theta*cosine_theta);
        view_light.x = sine_theta * cos(phi);
        view_light.y = sine_theta * sin(phi);
        view_light.z = cosine_theta;

        wx = view_light.x *  wy/view_light.y;
        wz = view_light.z *  wy/view_light.y;

        temp=view_light.x*c.x +view_light.y*c.y+view_light.z*c.z;
        xx = temp*temp +r*r -(c.x*c.x+c.y*c.y+c.z*c.z);

        if(fabs(wx) <= wmax && fabs(wz) <=wmax && xx > 0)
            break;
        }
        t = temp - sqrt(xx);
        len_s=0;
        len_n=0;
        I.x = t*view_light.x;
        s.x = L.x - I.x;
        n.x = I.x - c.x;
        len_s+=s.x*s.x;
        len_n+=n.x*n.x;

        I.y = t*view_light.y;
        s.y = L.y - I.y;
        n.y = I.y - c.y;
        len_s+=s.y*s.y;
        len_n+=n.y*n.y;

        I.z = t*view_light.z;
        s.z = L.z - I.z;
        n.z = I.z - c.z;
        len_s+=s.z*s.z;
        len_n+=n.z*n.z;
        len_s = sqrt(len_s);
        len_n = sqrt(len_n);
        
        b=0;
        s.x = s.x/len_s;
        n.x = n.x/len_n;
        b+=s.x*n.x;

        s.y = s.y/len_s;
        n.y = n.y/len_n;
        b+=s.y*n.y;

        s.z = s.z/len_s;
        n.z = n.z/len_n;
        b+=s.z*n.z;

        b = fmax(0, b);
        q = m*(wx + wmax)/(2*wmax);
        p = m*(wz + wmax)/(2*wmax);
        grid.get()[q][p] = grid.get()[q][p] + b;
    }
return grid;
}

// this function draw the sphere using N dots on the grid of size m, 
void draw()
{
    shared_ptr<T*>grid;
    double t0 = omp_get_wtime();
    if(double_thread)
        {
        std::promise<shared_ptr<T*>> promise;
        std::future<shared_ptr<T*>> future = promise.get_future();
        std::thread th([&]{ promise.set_value(ray_tracing());});
        grid = ray_tracing();
        shared_ptr<T*>grid_slave=future.get();
        //combine the result of main thread and slave thread
        for(int i=0;i<m;i++)
            for(int j=0;j<m;j++)
                grid.get()[i][j] += grid_slave.get()[i][j];
        th.join();
        }
    else
        grid = ray_tracing();
    
    double t1 = omp_get_wtime();
    time = t1-t0;

    //write the plot info into a file
    FILE* fp = fopen(filename,"w");
    if(fp == NULL) 
    {
    std::cout<<"Error opening file.\n";
    return;
    }
    for(int i=0;i<m;i++)
        {
        for(int j=0;j<m;j++)
            fprintf(fp, "%f ",grid.get()[i][j]);
        fprintf(fp, "\n");
        }
return;
}

    double time=0;
    private: 
    int m ;// size of the grid
    int N; // number of dots used to plot the sphere
    T r; // radius of the sphere
    T wy; // window's coordinate
    T wmax;// size of window
    coord<T> c; // coordinate of the sphere
    coord<T> L;// coordinate of the light source
    bool double_thread;
    const char* filename;
};

int
main(int argc, char**argv)
{
//m is the size of the grid used to present the sphere
int m = 1000;
//N is the number of points drawn on the grid
int N = 10000000;
double r = 6, wy=10, wmax=10;
//coordinate of sphere and light source
coord<double> c{0,12,0};
coord<double> L{4,4,-1};

srand(time(NULL));
const char* name1 = "grid1.txt";
const char* name2 = "grid2.txt";
// r1 in single thread mode. To benchmark, we need to set the number of dots to 2N
ray_tracer<double> r1(m,2*N,r,wy, wmax,c,L,false,name1);
// r2 in double thread mode
ray_tracer<double> r2(m,N,r,wy,wmax,c,L,true, name2);
r1.draw();
r2.draw();
std::cout<<"time used in single thread mode: "<< r1.time<<endl;
std::cout<<"time used in double thread mode: "<< r2.time<<endl;
return 0;
}
