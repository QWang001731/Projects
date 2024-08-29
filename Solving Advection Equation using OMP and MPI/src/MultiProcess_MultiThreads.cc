#include<cassert>
#include<cmath>
#include<fstream>
#include<iomanip>
#include<iostream>
#include<time.h>
#include<omp.h>
#include<mpi.h>
#include<cassert>
using namespace std;
#define DEBUG 0
#define WRITE_FILE 1

double** grid(int n){
    assert(n%2==0);
    int cols = n;
    int rows = n/2 + 2;
    double **m = (double**)malloc(rows*sizeof(double*));
    double *temp = (double*)malloc(rows*cols*sizeof(double));
    for(int i=0;i<rows;i++){
        m[i] = temp + n * i;
    }
    return m;
}

int main(int argc, char **argv){
    int nprocs;
    int mype;
    int stat;
    MPI_Status status;
    MPI_Init(&argc, &argv);
    stat = MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    assert(stat == MPI_SUCCESS);

    stat = MPI_Comm_rank(MPI_COMM_WORLD,&mype);
    assert(stat == MPI_SUCCESS);
    double t0 = omp_get_wtime();

if(mype==0){
    int * int_para = new int[2];
    double * double_para = new double[4];

    int_para[0] = atoi(argv[1]);
    int_para[1] = atoi(argv[2]);

    double_para[0] = atoi(argv[3]);
    double_para[1] = atoi(argv[4]);
    double_para[2] = atoi(argv[5]);
    double_para[3] = atoi(argv[6]);

    int N=int_para[0];
    int NT = int_para[1];
    double L = double_para[0];
    double T = double_para[1];
    double u = double_para[2];
    double v = double_para[3];
    int rows = N/2 + 2;

    MPI_Send(int_para,2,MPI_INT,1,0,MPI_COMM_WORLD);
    MPI_Send(double_para,4,MPI_DOUBLE,1,0,MPI_COMM_WORLD);
    double **Cn = grid(N);
    double **Cn_plus_1 = grid(N);
    double sigma_x = L/4;
    double sigma_y = L/4;
    double dx = L/N;
    double dt = T/NT;
    assert(dt < dx/(std::sqrt(2*(u*u+v*v))));
    double x,y;
    double **t;

    for(int i=1;i<rows-1;i++){
        x = -L/2. + dx * (i-1);
        for(int j=0;j<N;j++){
            y = -L/2. + dx * j;
            Cn[i][j] = exp(-100 * (x * x + y * y));
        }
    }

    int nthreads = omp_get_max_threads();
    for(int n=0;n<NT;n++){
        MPI_Recv(Cn[0],N,MPI_DOUBLE,1,100,MPI_COMM_WORLD,&status);
        MPI_Recv(Cn[rows-1],N,MPI_DOUBLE,1,200,MPI_COMM_WORLD,&status);
        MPI_Send(Cn[1],N,MPI_DOUBLE,1,300,MPI_COMM_WORLD);
        MPI_Send(Cn[rows-2],N,MPI_DOUBLE,1,400,MPI_COMM_WORLD);
        #pragma omp parallel for num_threads(nthreads) schedule(static)
        for(int i=1;i<rows-1;i++)
            for(int j=0;j<N;j++){
                int im1 = i-1;
                int ia1 = i+1;
                int jm1 = ((j-1)+N)%N;
                int ja1 = (j+1)%N;
                Cn_plus_1[i][j] = 0.25 * (Cn[im1][j] + Cn[ia1][j] + Cn[i][jm1] + Cn[i][ja1]) - 
                dt * (u * (Cn[ia1][j] - Cn[im1][j]) + v * (Cn[i][ja1] - Cn[i][jm1]))/(2 * dx);
            }

    t = Cn_plus_1;
    Cn_plus_1 = Cn;
    Cn = t;

    if(n%100==0&&DEBUG) std::cout<<"good on n="<<n<<std::endl;
    if(!WRITE_FILE) continue;
    if(n == 0&&omp_get_thread_num() == 0){
        FILE *data=fopen("init0.txt", "w");
        for(int i = 1; i < rows-1; i++){ 
            for(int j = 0; j < N; j++)  
                fprintf(data, "%.10f ",Cn[i][j]);
            fprintf(data, "\n");
        }

        if(fclose(data)!=0){
            perror("error closing file");
        }
    }
    
    if (n == NT/2&&omp_get_thread_num()==0){
        FILE *data=fopen("middle0.txt", "w");
        for(int i = 1; i<rows-1; i++){ 
            for(int j=0; j<N; j++)  
                fprintf(data,"%.10f ",Cn[i][j]);
            fprintf(data,"\n");
        }
        
        if(fclose(data)!=0){
            perror("error closing file");
        }
    }

    if (n == NT - 1 && omp_get_thread_num()==0){
        FILE *data=fopen("end0.txt", "w");
        for(int i = 1; i<rows-1; i++){ 
            for(int j=0; j<N; j++)  
                fprintf(data,"%.10f ",Cn[i][j]);
            fprintf(data,"\n");
        }
        
        if(fclose(data)!=0){
            perror("error closing file");
        }
    }    
    }

}

else if(mype==1){
    int * int_para = new int[2];
    double * double_para = new double[4];
    MPI_Recv(int_para,2,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    MPI_Recv(double_para,4,MPI_DOUBLE,0,0,MPI_COMM_WORLD,&status);
    int N=int_para[0];
    int NT = int_para[1];
    double L = double_para[0];
    double T = double_para[1];
    double u = double_para[2];
    double v = double_para[3];

    double **Cn = grid(N);
    double **Cn_plus_1 = grid(N);
    double sigma_x = L/4;
    double sigma_y = L/4;
    double dx = L/N;
    double dt = T/NT;
    assert(dt < dx/(std::sqrt(2*(u*u+v*v))));
    double x,y;
    double **t;

    int rows = N/2 + 2;

    for(int i=1;i<rows-1;i++){
        x = dx * (i - 1) ;
        for(int j=0;j<N;j++){
            y = -L/2. + dx * j;
            Cn[i][j] = exp(-100 * (x * x + y * y));
        }
    }

    int nthreads = omp_get_max_threads();
    for(int n=0;n<NT;n++){
        MPI_Send(Cn[rows-2],N,MPI_DOUBLE,0,100,MPI_COMM_WORLD);
        MPI_Send(Cn[1],N,MPI_DOUBLE,0,200,MPI_COMM_WORLD);
        MPI_Recv(Cn[rows-1],N,MPI_DOUBLE,0,300,MPI_COMM_WORLD,&status);
        MPI_Recv(Cn[0],N,MPI_DOUBLE,0,400,MPI_COMM_WORLD,&status);
        #pragma omp parallel for num_threads(nthreads) schedule(static)
        for(int i=1;i<rows-1;i++)
            for(int j=0;j<N;j++){
                int im1 = i-1;
                int ia1 = i+1;
                int jm1 = ((j-1)+N)%N;
                int ja1 = (j+1)%N;
                Cn_plus_1[i][j] = 0.25 * (Cn[im1][j] + Cn[ia1][j] + Cn[i][jm1] + Cn[i][ja1]) - 
                dt * (u * (Cn[ia1][j] - Cn[im1][j]) + v * (Cn[i][ja1] - Cn[i][jm1]))/(2 * dx);
            }

    t = Cn_plus_1;
    Cn_plus_1 = Cn;
    Cn = t;

    if(n%100==0&&DEBUG) std::cout<<"good on n="<<n<<std::endl;
    if(!WRITE_FILE) continue;
    if(n == 0&&omp_get_thread_num() == 0){
        FILE *data=fopen("init1.txt", "w");
        for(int i = 1; i < rows-1; i++){ 
            for(int j = 0; j < N; j++)  
                fprintf(data, "%.10f ",Cn[i][j]);
            fprintf(data, "\n");
        }

        if(fclose(data)!=0){
            perror("error closing file");
        }
    }
    
    if (n == NT/2&&omp_get_thread_num()==0){
        FILE *data=fopen("middle1.txt", "w");
        for(int i = 1; i<rows-1; i++){ 
            for(int j=0; j<N; j++)  
                fprintf(data,"%.10f ",Cn[i][j]);
            fprintf(data,"\n");
        }
        
        if(fclose(data)!=0){
            perror("error closing file");
        }
    }

    if (n == NT - 1 && omp_get_thread_num()==0){
        FILE *data=fopen("end1.txt", "w");
        for(int i = 1; i<rows-1; i++){ 
            for(int j=0; j<N; j++)  
                fprintf(data,"%.10f ",Cn[i][j]);
            fprintf(data,"\n");
        }
        
        if(fclose(data)!=0){
            perror("error closing file");
        }
    }    
    }


}

MPI_Finalize();
double t1 = omp_get_wtime();
std::cout<<"time:"<<t1-t0;
}
