#include"stdlib.h"
#include"stdio.h"
#include"omp.h"
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <cuda.h>
#include <curand.h>
#include <curand_kernel.h>
#define PI 3.1415926
#define MAX(a,b) ((a)>(b)? (a):(b))
#define MIN(a,b) ((a)<(b)? (a):(b))

double** grid_maker(int n){
    double* g = (double *)calloc(n*n, sizeof(double));
    double ** M = (double**)malloc(sizeof(double*)*n);
    for(int i=0;i<n;i++)
        M[i] = g + i * n;
    return M;
}

__global__ void update_grid(double *grid, long n, int m, int nrpt,unsigned long long* sample_device){
    long tid =  blockIdx.x * blockDim.x + threadIdx.x;
    for(int i = 0; i < nrpt && tid * i < n; i++){
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

int main(int argc, char**argv){
    int m=1000;
    long n = atoi(argv[1]);
    int ngrid = atoi(argv[2]);
    int nblocks = atoi(argv[3]);
    int nthreads = atoi(argv[4]);
    // number of rays per thread
    int nrpt = n/(nblocks * nthreads)+1;
    nblocks = MIN(nblocks, ngrid);
    double** grid_h = grid_maker(m);
    double* data_device;
    unsigned long long nsample=0;
    unsigned long long * sample_device;
    unsigned long long * sample_h=&nsample;

    cudaMalloc((void**) &data_device, m * m * sizeof(double));
    cudaMalloc((void**) &sample_device, sizeof(unsigned long long));
    cudaMemset(sample_device,0,sizeof(unsigned long long));

    float time_device;
    cudaEvent_t
    start_device,
    stop_device;
    cudaEventCreate(&start_device);
    cudaEventCreate(&stop_device);
    printf("launching kernel with %d blocks, %d threads per block, and %drays per thread\n",nblocks,nthreads,nrpt);
    double tt1 =omp_get_wtime();
    cudaEventRecord(start_device,0);
    update_grid<<<nblocks, nthreads>>>(data_device,n,m,nrpt,sample_device);
    cudaEventRecord(stop_device,0);
    cudaEventSynchronize(stop_device);
    cudaEventElapsedTime(&time_device,start_device,stop_device);
    cudaMemcpy(grid_h[0], data_device, m * m * sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(sample_h, sample_device, sizeof(unsigned long long), cudaMemcpyDeviceToHost);
    printf("kernel time %fs,\n ", time_device/1000.);

    FILE* fp = fopen("grid.txt","w");
    
    if(fp == NULL) {
        printf("Error opening file.\n");
        return 1;
    }

    for(int i=0;i<m;i++){
        for(int j=0;j<m;j++)
            fprintf(fp, "%f ",grid_h[i][j]);
        fprintf(fp, "\n");
    }

    free(grid_h[0]);
    free(grid_h);
    cudaFree(data_device);
    cudaEventDestroy(start_device);
    cudaEventDestroy(stop_device);
    double tt2 =omp_get_wtime();
    printf("total time %fs,\n ", tt2-tt1);
    printf("sample made %llu,\n ", nsample);
    return 0;
}
