#include<iostream>
#include<vector>
#include<cmath>
#include<fstream>
#include<string>
#include<omp.h>
using namespace std;

double ** grid(int N){
    double **g = (double**)malloc(N * sizeof(double*));
    double * h = (double*) malloc(N * N * sizeof(double));
    for(int i=0;i<N;i++)
        g[i] = (h + i * N);
    
    return g;
    }

int main(int argc, char**argv){

    int N = stoi(argv[1]),n=atoi(argv[2]);
    double L=1,T=1, u=1,v=1,dx=L /( N-1),dt= dx /(2* sqrt(2.0));
    double ** Cn=grid(N);
    double ** Cn1=grid(N);
    int NT = T/dt;

    cout<<"parameters:\nN="<<N<<" NT="<<NT<<" L="<<L<<
    " T="<<T<<" u="<<u<<" v="<<v<<" dx="<<dx<<" dt="<<dt<<" num_threads="<<n<<endl;
    auto t1=omp_get_wtime();
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
    grid.close();

    int im1,ia1,jm1,ja1;;

    for(int t=0;t<NT;t++){

        #pragma omp parallel for num_threads(n)
        for(int i=0;i<N;i++){
            if (i==0)   
                im1=N-1;
            else 
                im1=i-1;
            if (i==N-1) 
                ia1=0;
            else 
                ia1=i+1; 
            
            for(int j=0;j<N;j++){
                if (j==0)  
                    jm1=N-1; 
                else  
                    jm1=j-1;
                if (j==N-1)  
                    ja1=0; 
                else
                  ja1=j+1;
                Cn1[i][j] = (Cn[im1][j] + Cn[ia1][j] + Cn[i][jm1] + Cn[i][ja1])/4 - 
                            (dt /(2 * dx)) * (u * (Cn[ia1][j] - Cn[im1][j]) + v * (Cn[i][ja1]-Cn[i][jm1]));
                }
            }
        
        double **tt = Cn;
        Cn=Cn1;
        Cn1=tt;

        if(t==NT/2){
            cout<<"thread "<<omp_get_thread_num()<<" is plotting middle\n";
            ofstream middle("middle.txt");
            for(int i=0;i<N;i++){
                for(int j=0;j<N;j++)
                    middle<<Cn[i][j]<<" ";
                middle<<endl;
                }
            
            middle.close();
            }
        if(t==NT-1){
            cout<<"thread "<<omp_get_thread_num()<<" is plotting final\n";
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
    cout<<"time:"<<((double)(t2-t1));
    }