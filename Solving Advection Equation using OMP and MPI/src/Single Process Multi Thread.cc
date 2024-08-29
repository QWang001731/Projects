#include<cassert>
#include<cmath>
#include<fstream>
#include<iomanip>
#include<iostream>
#include<time.h>
#include<omp.h>
#include<mpi.h>
using namespace std;
#define DEBUG 0

double** grid(int n){
    double **m = (double**)malloc(n*sizeof(double*));
    double *temp = (double*)malloc(n*n*sizeof(double));
    for(int i=0;i<n;i++){
        m[i] = temp + n * i;
    }
    return m;
}

int main(int argc, char**argv){
    int N = atoi(argv[1]);
    int NT = atoi(argv[2]);
    double L = atoi(argv[3]);
    double T = atoi(argv[4]);
    double u = atoi(argv[5]);
    double v = atoi(argv[6]);
    double **Cn = grid(N);
    double **Cn_plus_1 = grid(N);
    double sigma_x = L/4;
    double sigma_y = L/4;
    double dx = L/N;
    double dt = T/NT;
    assert(dt < dx/(std::sqrt(2*(u*u+v*v))));
    double x,y;
    double **t;

    for(int i=0;i<N;i++){
        x = -L/2.0 + dx * i;
        for(int j=0;j<N;j++){
            y = -L/2.0 + dx * j;
            Cn[i][j] = exp(-.5 * (x * x + y * y));
        }
    }

    double t0 = omp_get_wtime();
    int nthreads = omp_get_max_threads();
    
    #pragma omp parallel for num_threads(nthreads) schedule(static)
    for(int n=0;n<NT;n++){
        for(int i=0;i<N;i++)
            for(int j=0;j<N;j++){
                int im1 = ((i-1)+N)%N;
                int ia1 = (i+1)%N;
                int jm1 = ((j-1)+N)%N;
                int ja1 = (j+1)%N;
                Cn_plus_1[i][j] = 0.25 * (Cn[im1][j] + Cn[ia1][j] + Cn[i][jm1] + Cn[i][ja1]) - 
                dt * (u * (Cn[ia1][j] - Cn[im1][j]) + v * (Cn[i][ja1] - Cn[i][jm1]))/(2 * dx);
            }

    t = Cn_plus_1;
    Cn_plus_1 = Cn;
    Cn = t;

    if(n%100==0&&DEBUG) std::cout<<"good on n="<<n<<std::endl;
    if(!DEBUG) continue;
    if(n == 0&&omp_get_thread_num() == 0){
        FILE *data=fopen("init.txt", "w");
        for(int i = 0; i < N; i++){ 
            for(int j = 0; j < N; j++)  
                fprintf(data, "%.10f ",Cn[i][j]);
            fprintf(data, "\n");
        }

        if(fclose(data)!=0){
            perror("error closing file");
        }
    }
    
    if (n == NT/2&&omp_get_thread_num()==0){
        FILE *data=fopen("middle.txt", "w");
        for(int i = 0; i<N; i++){ 
            for(int j=0; j<N; j++)  
                fprintf(data,"%.10f ",Cn[i][j]);
            fprintf(data,"\n");
        }
        
        if(fclose(data)!=0){
            perror("error closing file");
        }
    }

    if (n == NT - 1 && omp_get_thread_num()==0){
        FILE *data=fopen("end.txt", "w");
        for(int i = 0; i<N; i++){ 
            for(int j=0; j<N; j++)  
                fprintf(data,"%.10f ",Cn[i][j]);
            fprintf(data,"\n");
        }
        
        if(fclose(data)!=0){
            perror("error closing file");
        }
    }    
}
    double t1 = omp_get_wtime();
    printf("time with %d threads:%.2f seconds\n",nthreads, t1-t0);
    
    free(Cn[0]);
    free(Cn);
    free(Cn_plus_1[0]);
    free(Cn_plus_1);
}