#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include "cublas_v2.h"
#include "mnist.h"

void initialize_W_and_B_on_host(double* W[], double*B[],int L, int nhidden)
{
W[0]=NULL;
B[0]=NULL;
W[1]= (double*)malloc(SIZE* nhidden * sizeof(double));
B[1]= (double*)malloc(nhidden * sizeof(double));

double stddev = sqrt(2.0/SIZE);
for(int i=0;i<SIZE*nhidden;i++)
  W[1][i] = stddev * ((double) rand() / (RAND_MAX / 2.0f) - 1.0f);

stddev = sqrt(2.0/nhidden);
for(int i=0;i<nhidden;i++)
  B[1][i] = stddev * ((double) rand() / (RAND_MAX / 2.0f) - 1.0f);

for(int i=2;i <L-1 ;i++)
{
  W[i] = (double*)malloc(nhidden* nhidden * sizeof(double));
  for(int j=0;j<nhidden*nhidden;j++)
      W[i][j] = stddev * ((double) rand() / (RAND_MAX / 2.0f) - 1.0f);
  
  B[i] = (double*)malloc(nhidden * sizeof(double));
  for(int j=0;j<nhidden;j++)
      B[i][j] = stddev * ((double) rand() / (RAND_MAX / 2.0f) - 1.0f);

}

W[L-1]=(double*)malloc( OUT_NUM* nhidden * sizeof(double));
  for(int j=0;j<OUT_NUM*nhidden;j++)
      W[L-1][j] = stddev * ((double) rand() / (RAND_MAX / 2.0f) - 1.0f);

B[L-1]=(double*)malloc(OUT_NUM * sizeof(double));
stddev = sqrt(2.0/OUT_NUM);
for(int j=0;j<OUT_NUM;j++)
    B[L-1][j] = stddev * ((double) rand() / (RAND_MAX / 2.0f) - 1.0f);
}

void initialize_W_and_B_on_GPU(double* W_h[], double*B_h[],double* W[], double*B[],int L, int nhidden)
{
W[0] = NULL;
B[0] = NULL; 
cudaMalloc((void **) &W[1], SIZE*nhidden*sizeof(double));
cudaMemcpy(W[1],W_h[1],SIZE*nhidden*sizeof(double),cudaMemcpyHostToDevice);
cudaMalloc((void **) &B[1], nhidden*sizeof(double));
cudaMemcpy(B[1],B_h[1],nhidden*sizeof(double),cudaMemcpyHostToDevice);

for(int i=2;i<L-1;i++)
{
cudaMalloc((void **) &W[i], nhidden * nhidden * sizeof(double));
cudaMemcpy(W[i], W_h[i], nhidden * nhidden * sizeof(double),cudaMemcpyHostToDevice);
cudaMalloc((void **) &B[i], nhidden * sizeof(double));
cudaMemcpy(B[i], B_h[i], nhidden * nhidden * sizeof(double),cudaMemcpyHostToDevice);
}

cudaMalloc((void **) &W[L-1], OUT_NUM * nhidden * sizeof(double));
cudaMemcpy(W[L-1], W_h[L-1], nhidden * nhidden * sizeof(double),cudaMemcpyHostToDevice);
cudaMalloc((void **) &B[L-1], OUT_NUM * sizeof(double));
cudaMemcpy(B[L-1], B_h[L-1], OUT_NUM * sizeof(double),cudaMemcpyHostToDevice);
}

int
main()
{double alpha1 = 1;
double beta = 0;
int nlayer = 1;
int nhidden = 800, nepoch = 3, nbatch = 256;
double alpha = 0.01;
int L = nlayer + 2;

double * W_h[L], *B_h[L], *Wu_h[L], *Bu_h[L];
double * W[L],   *B[L],   *Wu[L],   *Bu[L];

initialize_W_and_B_on_host(W_h, B_h,L,nhidden);
initialize_W_and_B_on_host(Wu_h, Bu_h, L,nhidden);
initialize_W_and_B_on_GPU(W_h, B_h,W,B,L,nhidden);
initialize_W_and_B_on_GPU(Wu_h,Bu_h,Wu,Bu,L,nhidden);


//cublasHandle_t handle;
//cublasCreate(&handle);
//cublasDgemm(handle,CUBLAS_OP_N,CUBLAS_OP_N,M, N, K,&alpha1, a, M, b, K,& beta,c,M);
//cublasDestroy(handle);
return 0;
}
