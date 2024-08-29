#include"stdlib.h"
#include"stdio.h"
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <cuda.h>
#include <curand.h>
#include<curand_kernel.h>
#include "omp.h"
#define PI 3.1415926
#define MAX(a,b) ((a)>(b)? (a):(b))
#define MIN(a,b) ((a)<(b)? (a):(b))

__global__ void update_grid(double *grid, long n, int m, int nrpt,unsigned long long* sample_device){
    long tid =  blockIdx.x * blockDim.x + threadIdx.x;
    for(long i = 0; i < nrpt && tid * i < n; i++ ){
        double wy=10, wmax=10, r=6, len_s=0, len_n=0,wx, wz,temp,t,b,xx;
        double c[3] = {0,12,0};
        double I[3];
        double L[3] = {4,4,-1};
        double s[3];
        double n[3];
        double view_light[3],phi,cosine_theta, sine_theta;
        int p,q;
        unsigned  long long one=1;
	    unsigned int tid0 = blockIdx.x * blockDim.x + threadIdx.x ;
	    unsigned int seed = tid0 * 3419;
        curandStateMRG32k3a state;
        curand_init(seed, 0, 0, &state);
        while(true){
            atomicAdd(sample_device, one);
            phi = PI * curand_uniform(&state);
            cosine_theta = (curand_uniform(&state) - 0.5) * 2;
            sine_theta = sqrt(1-cosine_theta*cosine_theta);
            view_light[0] = sine_theta * cos(phi);
            view_light[1] = sine_theta * sin(phi);
            view_light[2] = cosine_theta;

            wx = view_light[0] *  wy/view_light[1];
            wz = view_light[2] *  wy/view_light[1];

            temp=view_light[0]*c[0]+view_light[1]*c[1]+view_light[2]*c[2];
            xx = temp*temp +r*r -(c[0]*c[0]+c[1]*c[1]+c[2]*c[2]);

            if(fabs(wx) <= wmax && fabs(wz) <=wmax && xx > 0)
                break;
        }
        t = temp - sqrt(xx);
        len_s=0;
        len_n=0;
        for(int i=0;i<3;i++){
            I[i] = t*view_light[i];
            s[i] = L[i] - I[i];
            n[i] = I[i] - c[i];
            len_s+=s[i]*s[i];
            len_n+=n[i]*n[i];
        }
        len_s = sqrt(len_s);
        len_n = sqrt(len_n);
        b=0;
        for(int i=0;i<3;i++){
            s[i] = s[i]/len_s;
            n[i] = n[i]/len_n;
            b+=s[i]*n[i];
        }
        b = MAX(0, b);
        q = m*(wx + wmax)/(2*wmax);
        p = m*(wz + wmax)/(2*wmax);
        grid[q*m+p] = grid[q*m+p] + b;
    }
    return;
}

extern "C" void mpi_call(int n, int nblocks, int nthreads, int ngrid, double** grid_h,unsigned long long * sample_h, float* time_host,int m,int mype){
    int nrpt = n/(nblocks * nthreads)+1;
    nblocks = MIN(nblocks, ngrid);
    double* data_device;
    unsigned long long * sample_device;
    cudaMalloc((void**) &data_device, m * m * sizeof(double));
    cudaMalloc((void**) &sample_device, sizeof(unsigned long long));
    cudaMemset(sample_device, 0, sizeof(unsigned long long));
    cudaEvent_t
    start_device,
    stop_device;
    cudaEventCreate(&start_device);
    cudaEventCreate(&stop_device);
    printf("node %i: launching kernel with %d blocks, %d threads per block, and %drays per thread\n",mype,nblocks,nthreads,nrpt);
    cudaEventRecord(start_device,0);
    update_grid<<<nblocks, nthreads>>>(data_device,n,m,nrpt,sample_device);
    cudaEventRecord(stop_device,0);
    cudaEventSynchronize(stop_device);
    cudaEventElapsedTime(time_host,start_device,stop_device);
    cudaMemcpy(grid_h[0], data_device, m * m * sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(sample_h, sample_device, sizeof(unsigned long long), cudaMemcpyDeviceToHost);
    cudaFree(data_device);
    cudaEventDestroy(start_device);
    cudaEventDestroy(stop_device);
    return ;
}


