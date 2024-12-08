#include<iostream>
#include<vector>
#include<cmath>
#include<fstream>
#include<string>
#include<mpi.h>
#include<omp.h>
using namespace std;

double ** grid(int M, int N){
    double **g = (double**)malloc(M * sizeof(double*));
    double * h = (double*) malloc(M * N * sizeof(double));
    for(int i=0;i<M;i++)    
        g[i] = h + N * i;
    
    return g;
    }

int main(int argc, char**argv){

    MPI_Init(&argc, &argv);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    int N = atoi(argv[1]),
        n=atoi(argv[2]);
    double L=1,T=1, u=1,v=1,dx=L/(N-1),dt= dx/(2* sqrt(2.0));
    int NT = T/dt;
    if(rank==0){
        auto t1=omp_get_wtime();
        double ** Cn=grid(N, N);
        double ** Cn1=grid(N, N);
        #pragma omp parallel for num_threads(n)
        for(int i=0;i<N;i++){
            double x = dx * i - L/2;
            for(int j=0;j<N;j++){
                double y = dx * j - L/2;
                double t = x * x + y * y;
                Cn[i][j] = exp(-100 * t);
                }
            }

        ofstream grid("initial.txt");
        for(int i=0;i<N;i++){
            for(int j=0;j<N;j++)
                grid<<Cn[i][j]<<" ";
            grid<<endl;
            }
        
        MPI_Send(Cn[N/4-1], N * (N/4 + 2), MPI_DOUBLE, 1, 1, MPI_COMM_WORLD);
        MPI_Send(Cn[N/2-1], N * (N/4 + 2),  MPI_DOUBLE, 2, 1, MPI_COMM_WORLD);
        MPI_Send(Cn[3 * N/4 - 1], N * (N/4 + 1), MPI_DOUBLE, 3, 1, MPI_COMM_WORLD);
        MPI_Send(Cn[0],N, MPI_DOUBLE, 3, 1, MPI_COMM_WORLD);

    for(int iter=0;iter<NT;iter++){
        #pragma omp parallel for num_threads(n)
        for(int i=0;i<N/4;i++){
            int im1;
            if (i==0)   
                im1=N-1;
            else 
                im1=i-1;

            for(int j=0;j<N;j++){
                int jm1,ja1;
                if (j==0)  
                    jm1=N-1; 
                else  
                    jm1=j-1;
                if (j==N-1)  
                    ja1=0; 
                else
                    ja1=j+1;
                Cn1[i][j] = (Cn[im1][j] + Cn[i+1][j] + Cn[i][jm1] + Cn[i][ja1])/4 -
                (dt /(2 * dx)) * (u * (Cn[i+1][j] - Cn[im1][j]) + v * (Cn[i][ja1]-Cn[i][jm1]));
            }
        }

        MPI_Recv(Cn1[N/4],N,MPI_DOUBLE,1,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Send(Cn[N/4-1],N,MPI_DOUBLE,1,3,MPI_COMM_WORLD);

        MPI_Recv(Cn1[N-1],N,MPI_DOUBLE,3,3,MPI_COMM_WORLD,MPI_STATUSES_IGNORE);
        MPI_Send(Cn[0],N,MPI_DOUBLE, 3,3,MPI_COMM_WORLD);

        double **tt = Cn;
        Cn=Cn1;
        Cn1=tt;

        if(iter==NT/2){
            MPI_Recv(Cn[N/4], N*N/4, MPI_DOUBLE,1,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(Cn[N/2], N*N/4, MPI_DOUBLE,2,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(Cn[3*N/4], N*N/4, MPI_DOUBLE,3,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            ofstream middle("middle.txt");
            for(int i=0;i<N;i++){
                for(int j=0;j<N;j++)
                    middle<<Cn[i][j]<<" ";
                middle<<endl;
                }
            middle.close();
            }

        if(iter==NT-1){
            MPI_Recv(Cn[N/4], N*N/4, MPI_DOUBLE,1,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(Cn[N/2], N*N/4, MPI_DOUBLE,2,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(Cn[3*N/4], N*N/4, MPI_DOUBLE,3,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            ofstream final("final.txt");
            for(int i=0;i<N;i++){
                for(int j=0;j<N;j++)
                    final<<Cn[i][j]<<" ";
                final<<endl;
            }
            final.close();
            }

    }

    auto t2=omp_get_wtime();
    cout<<"time:"<<(double)(t2-t1)<<endl;   
    }else if(rank==1){
        double **Cn  = grid(N/4+2,N);
        double **Cn1 = grid(N/4+2,N);
        MPI_Recv(Cn[0],N * (N/4 + 2),MPI_DOUBLE,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        for(int iter=0;iter<NT;iter++){
            #pragma omp parallel for num_threads(n)
            for(int i=1;i<=N/4;i++){
                for(int j=0;j<N;j++){
                    int jm1,ja1;
                    if (j==0)  
                        jm1=N-1; 
                    else  
                        jm1=j-1;
                    if (j==N-1)  
                        ja1=0; 
                    else
                        ja1=j+1;

                    Cn1[i][j] = (Cn[i-1][j] + Cn[i+1][j] + Cn[i][jm1] + Cn[i][ja1])/4 - 
                    (dt /(2 * dx)) * (u * (Cn[i+1][j] - Cn[i-1][j]) + v * (Cn[i][ja1]-Cn[i][jm1]));
                }
            }
            MPI_Send(Cn[1],N,MPI_DOUBLE,0,3,MPI_COMM_WORLD);
            MPI_Recv(Cn1[0],N,MPI_DOUBLE,0,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

            MPI_Recv(Cn1[N/4+1],N,MPI_DOUBLE,2,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Send(Cn[N/4],N,MPI_DOUBLE,2,3,MPI_COMM_WORLD);

            if(iter==NT/2)
                MPI_Send(Cn1[1],N*N/4,MPI_DOUBLE,0,1,MPI_COMM_WORLD);
            
            if(iter==NT-1)
                MPI_Send(Cn1[1],N*N/4,MPI_DOUBLE,0,2,MPI_COMM_WORLD);
            
            double **tt = Cn;
            Cn=Cn1;
            Cn1=tt;

        }

    }else if(rank==2){
        double **Cn = grid(N/4+2,N);
        double **Cn1 = grid(N/4+2,N);
        MPI_Recv(Cn[0],N*(N/4+2),MPI_DOUBLE,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        for(int iter=0;iter<NT;iter++){
            #pragma omp parallel for num_threads(n)
            for(int i=1;i<=N/4;i++){
                for(int j=0;j<N;j++){
                    int jm1,ja1;
                    if (j==0)  
                        jm1=N-1; 
                    else  
                        jm1=j-1;
                    if (j==N-1)  
                        ja1=0; 
                    else
                        ja1=j+1;
                    Cn1[i][j] = (Cn[i-1][j] + Cn[i+1][j] + Cn[i][jm1] + Cn[i][ja1])/4 - 
                    (dt /(2 * dx)) * (u * (Cn[i+1][j] - Cn[i-1][j]) + v * (Cn[i][ja1]-Cn[i][jm1]));
                }
            }
            MPI_Send(Cn[1],N,MPI_DOUBLE,1,3,MPI_COMM_WORLD);
            MPI_Recv(Cn1[0],N,MPI_DOUBLE,1,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

            MPI_Recv(Cn1[N/4+1],N,MPI_DOUBLE,3,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Send(Cn[N/4],N,MPI_DOUBLE,3,3,MPI_COMM_WORLD);


            if(iter==NT/2){
                MPI_Send(Cn1[1],N*N/4,MPI_DOUBLE,0,1,MPI_COMM_WORLD);
            }
            if(iter==NT-1)
                MPI_Send(Cn1[1],N*N/4,MPI_DOUBLE,0,2,MPI_COMM_WORLD);
            
            double **tt = Cn;
            Cn=Cn1;
            Cn1=tt;
        }

    }else if(rank==3){

        double **Cn = grid(N/4+2,N);
        double **Cn1 = grid(N/4+2,N);
        MPI_Recv(Cn[0], N * (N/4+1),MPI_DOUBLE,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(Cn[N/4+1],N,MPI_DOUBLE,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

        for(int iter=0;iter<NT;iter++){
            #pragma omp parallel for num_threads(n)
            for(int i=1;i<=N/4;i++){
                for(int j=0;j<N;j++){
                    int jm1,ja1;
                    if (j==0)  
                        jm1=N-1; 
                    else  
                        jm1=j-1;
                    if (j==N-1)  
                        ja1=0; 
                    else
                        ja1=j+1;
                    Cn1[i][j]=(Cn[i-1][j] + Cn[i+1][j] + Cn[i][jm1] + Cn[i][ja1])/4 - 
                    (dt /(2 * dx)) * (u * (Cn[i+1][j] - Cn[i-1][j]) + v * (Cn[i][ja1]-Cn[i][jm1]));
                }
            }

            MPI_Send(Cn[N/4],N,MPI_DOUBLE,0,3,MPI_COMM_WORLD);
            MPI_Recv(Cn1[N/4+1],N,MPI_DOUBLE,0,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

            MPI_Send(Cn[0],N,MPI_DOUBLE,2,3,MPI_COMM_WORLD);
            MPI_Recv(Cn1[1],N,MPI_DOUBLE,2,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

            if(iter==NT/2)
                MPI_Send(Cn1[1],N*N/4,MPI_DOUBLE,0,1,MPI_COMM_WORLD);
            
            if(iter==NT-1)
                MPI_Send(Cn1[1],N*N/4,MPI_DOUBLE,0,2,MPI_COMM_WORLD);

            double **tt = Cn;
            Cn=Cn1;
            Cn1=tt;
        }
    }

    MPI_Finalize();
    return 0;
    }
