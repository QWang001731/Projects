#include "neuralNetwork.hpp"
#include <memory.h>
#include <math.h>
#define max(a, b) ((a) > (b))? (a):(b)
#define NUM_LABEL 10
#define DEBUG 0
double Relu(double x)
{return ((x) > (0))? (x):(0);
}
double ReluPrime(double x)
{
    return x > 0? 1:0;
}
void softmax(double * arr, int rows, int cols){
for (int i=0;i<cols;i++)
    {
    double sum=0;
    for(int j=0;j<rows;j++)
        sum += exp(arr[j*cols+i]);
    for(int j=0;j<rows;j++)
        arr[j*cols+i] = exp(arr[j*cols+i])/sum;
    }
}

void fisher_yates_shuffle(int *arr, int n)
{
srand(time(NULL));
for(int i=0;i<n;i++)
        arr[i]=i;
for(int i=n-1;i>0;i--)
    {int j = rand()%(i+1);
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
    }
}
template <class T>
T neuralNetwork<T>::sigmoid(T z)
{
return 1.0f/(1+exp(-z));
}

template <class T>
void neuralNetwork<T>::train_CPU(double* img, double* labelVector, 
int numTrain, unsigned char* labelsforval,T alpha,int niters){
int* randNum = (int*)malloc(numTrain*sizeof(int));
double* batch = (double*)malloc(sizeof(double)*InputDim*BatchSize);
double* labels = (double*)malloc(sizeof(double)*NUM_LABEL*BatchSize);
int nbatch = int(numTrain/BatchSize);
for(int iter=0;iter<niters;iter++)
{fisher_yates_shuffle(randNum, numTrain);
std::cout<<"Training started...\n"
if(DEBUG)
{
    double t0=0,t1=0,t2=0,t3=0,t4=0,t5=0,t6=0,t7=0;
    for(int i=0;i<OutNodes*BatchSize;i++)
        t0+=Zout[i];
    for(int i=0;i<OutNodes*BatchSize;i++)
        t1+=out[i];
    for(int i=0;i<InputDim*NodesPerLayer;i++)
        t2+=W1[i];
    for(int i=0;i<NodesPerLayer;i++)
        t3+=Bhidden[0][i];
    for(int i=0;i<OutNodes*NodesPerLayer;i++)
        t4+=Wout[i];
    for(int i=0;i<OutNodes;i++)
        t5+=Bout[i];
    
    printf("sum W1:%f, sum B1:%f, sum Wout:%f, sum Bout:%f\n",t2,t3,t4,t5);
    printf("Error: %.1f", validate(&img[50000*784],&labelsforval[50000]));
}

for(int b=0;b<nbatch;b++)
    {
    int*start=&randNum[b * BatchSize];
    for(int i=0;i<BatchSize;i++)
        for(int j=0;j<InputDim;j++)
            batch[j*BatchSize+i] = img[start[i]*InputDim+j];

    for(int i=0;i<BatchSize;i++)
        for(int j=0;j<NUM_LABEL;j++)
            labels[j*BatchSize+i] = labelVector[start[i] + j*60000];
    
    //start forward propagate
    cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,
    NodesPerLayer,BatchSize,InputDim,
    1,W1,InputDim,batch,BatchSize,0,Z[0],BatchSize);
    for(int i=0;i<NodesPerLayer*BatchSize;i++)
        {
            int j =i/BatchSize;
            Z[0][i] += Bhidden[0][j];
            A[0][i] = Relu(Z[0][i]);
        }
    
    for(int i=1;i<=NHiddenLayers-1;i++)
        {
        cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans, NodesPerLayer,
        BatchSize,NodesPerLayer,1,Whidden[i-1],NodesPerLayer,A[i-1],BatchSize,
        0,Z[i],BatchSize);
        for(int j=0;j<NodesPerLayer*BatchSize;j++)
            {
                Z[i][j] = Z[i][j] + Bhidden[i][j];
                A[i][j] = Relu(Z[i][j]);
            }
        }
        cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans, OutNodes,BatchSize,
        NodesPerLayer,1,Wout,NodesPerLayer,A[NHiddenLayers-1],BatchSize,0,
        Zout,BatchSize);
        
        for(int i=0;i<OutNodes*BatchSize;i++)
            {int j = i/BatchSize;
            out[i] = Zout[i] + Bout[j];
            }
        softmax(out,OutNodes,BatchSize);
    
    // start backpropagate
    for(int i=0;i<OutNodes*BatchSize;i++)
        deltal[NHiddenLayers][i] = (out[i] - labels[i])*ReluPrime(Zout[i]);
    //compute uWout
    cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasTrans,OutNodes,NodesPerLayer,
    BatchSize,1,deltal[NHiddenLayers],BatchSize,A[NHiddenLayers-1],
    BatchSize,0,uWout,NodesPerLayer);
    
    //update Wout 
    for(int i=0;i<NodesPerLayer*OutNodes;i++)
        Wout[i] = Wout[i] - (alpha/BatchSize)*uWout[i];

    //compute uBout
    for(int i=0;i<OutNodes;i++)
       {
        deltal1d[NHiddenLayers][i]=0;
        for(int j=0;j<BatchSize;j++)
            deltal1d[NHiddenLayers][i]+= deltal[NHiddenLayers][i*BatchSize+j];
        }
    
    //update Bout
    for(int i=0;i<OutNodes;i++)
       Bout[i] = Bout[i] - (alpha/BatchSize)*deltal1d[NHiddenLayers][i];

    
    if(NHiddenLayers>1){
        //compute deltal[Nhidden-1]
    cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, NodesPerLayer,
    BatchSize,OutNodes,1,Wout,NodesPerLayer,deltal[NHiddenLayers],BatchSize,0,
    deltal[NHiddenLayers-1], BatchSize);
    for(int i=0;i<NodesPerLayer*BatchSize;i++)
        deltal[NHiddenLayers-1][i] = deltal[NHiddenLayers-1][i]* ReluPrime(Z[NHiddenLayers-1][i]);
    
    //compute uWhidden[Nhidden-1]
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasTrans, NodesPerLayer,NodesPerLayer,
    BatchSize,1,deltal[NHiddenLayers-1],BatchSize, A[NHiddenLayers-2],BatchSize,0,
    uWhidden[NHiddenLayers-2],NodesPerLayer);

    //update Whidden[Nhidden-1]
    for(int i=0;i<NodesPerLayer*NodesPerLayer;i++)
        Whidden[NHiddenLayers-2][i]=Whidden[NHiddenLayers-2][i]-(alpha/BatchSize)*uWhidden[NHiddenLayers-2][i];
    
    //update Bhidden[Nhidden-1]
    for(int i=0;i<NodesPerLayer;i++)
        for(int j=0;j<BatchSize;j++)
            deltal1d[NHiddenLayers-1][i]+= double(deltal[NHiddenLayers-1][i*BatchSize+j]/BatchSize);
    
    for(int i=0;i<NodesPerLayer;i++)
        Bhidden[NHiddenLayers-1][i] = Bhidden[NHiddenLayers-1][i] - (alpha/BatchSize)*deltal1d[NHiddenLayers-1][i];
    }
    //compute Whidden middle terms
    for(int i=NHiddenLayers-3;i>=1;i--)
        {
        //compute delta[i]
        cblas_dgemm(CblasRowMajor,CblasTrans,CblasNoTrans, NodesPerLayer,BatchSize,
        NodesPerLayer,1,Whidden[i+1],NodesPerLayer,deltal[i+1],BatchSize,
        0,deltal[i],BatchSize);
        
        for(int j=0;j<NodesPerLayer*BatchSize;j++)
            deltal[i][j] = deltal[i][j]*ReluPrime(Z[i][j]);
        
        //compute uWhidden[i]
        cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasTrans,NodesPerLayer,NodesPerLayer,BatchSize,
        1, deltal[i],BatchSize,A[i-1],BatchSize,0,uWhidden[i],NodesPerLayer);

        for(int j=0;j<NodesPerLayer*NodesPerLayer;j++)
            Whidden[i][j]=Whidden[i][j]-(alpha/BatchSize)*uWhidden[i][j];

    //update Bhidden[Nhidden-1]
    for(int ii=0;ii<NodesPerLayer;ii++)
        for(int jj=0;jj<BatchSize;jj++)
            deltal1d[i][ii]+= double(deltal[i][ii*BatchSize+jj]/BatchSize);
    
    for(int j=0;j<NodesPerLayer;j++)
        Bhidden[i][j] = Bhidden[i][j] - (alpha/BatchSize)*deltal1d[i][j];
        }
   
    //compute delta[0]
        if (NHiddenLayers>1)
        cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, NodesPerLayer,
        BatchSize,NodesPerLayer,1,Whidden[0],NodesPerLayer,deltal[1],BatchSize,0,
        deltal[0], BatchSize);
        else
        cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, NodesPerLayer,
        BatchSize,OutNodes,1,Wout,NodesPerLayer,deltal[1],BatchSize,0,
        deltal[0], BatchSize);

        for(int i=0;i<NodesPerLayer*BatchSize;i++)
            deltal[0][i] = deltal[0][i]* ReluPrime(Z[0][i]);
        
        //compute uW1
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasTrans, NodesPerLayer,InputDim,
        BatchSize,1,deltal[0],BatchSize,batch,BatchSize,0,uW1,InputDim);
    
        for(int i=0;i<InputDim*NodesPerLayer;i++)
            W1[i]= W1[i]-(alpha/BatchSize)*uW1[i];
      
        //compute uB1
        for(int i=0;i<NodesPerLayer;i++)
            {   deltal1d[0][i] = 0;
                for(int j=0;j<BatchSize;j++)
                deltal1d[0][i] += deltal[0][i*BatchSize+j];
            }
        //update B1
        for(int i=0;i<NodesPerLayer;i++)
           Bhidden[0][i] = Bhidden[0][i] - (alpha/BatchSize)*deltal1d[0][i];
    }
}



free(labels);
free(batch);
free(randNum);
printf("after traininig, we have Whidden[0]:\n");

if(NHiddenLayers>1)
{
for(int i=0;i<NodesPerLayer*NodesPerLayer;i++)
    {printf("%f, ", Whidden[0][i]);
        if(i%10==9)
            printf("\n");
    }
}
for(int i=0;i<NodesPerLayer*NodesPerLayer;i++)
    {printf("%f, ", W1[i]);
        if(i%10==9)
            printf("\n");
    }

return;
}
template class neuralNetwork<double>;

template<class T>
void neuralNetwork<T>::forward(unsigned char* img, size_t numTrain)
{
return;
}

template<class T>
int neuralNetwork<T>::predict(double * img)
{   double* ZZ = (double*) malloc(sizeof(double)*NodesPerLayer);
    double* AA = (double*) malloc(sizeof(double)*NodesPerLayer);
    double* ZZout = (double*) malloc(sizeof(double)*OutNodes);
    double* Oout = (double*) malloc(sizeof(double)*OutNodes);
    cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasTrans,NodesPerLayer,
    1,InputDim,1,W1,InputDim,img,InputDim,0,ZZ,1);
    for(int i=0;i<NodesPerLayer;i++)
        {
            
            ZZ[i] = ZZ[i] + Bhidden[0][i];
            AA[i] = Relu(ZZ[i]);
        }
    
    for(int i=1;i<=NHiddenLayers-1;i++)
        {
        cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans, NodesPerLayer,
        BatchSize,NodesPerLayer,1,Whidden[i-1],NodesPerLayer,A[i-1],BatchSize,
        0,Z[i],BatchSize);
        for(int j=0;j<NodesPerLayer*BatchSize;j++)
            {
                Z[i][j] = Z[i][j] + Bhidden[i][j];
                A[i][j] = Relu(Z[i][j]);
            }
        }

        cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans, OutNodes,1,
        NodesPerLayer,1,Wout,NodesPerLayer,AA,1,0,
        ZZout,1);
        for(int i=0;i<OutNodes;i++)
            Oout[i] = ZZout[i] + Bout[i];
        softmax(Oout,OutNodes,1);
        for(int i=0;i<NUM_LABEL;i++)
            printf("%f,\n",Oout[i]);
        int result;
        double Max = -1.0;
        for(int j=0;j<NUM_LABEL;j++)
            {if (Oout[j] > Max)
                {
                Max = Oout[j];
                result=j;
                }
            }
            return result;
}

template<class T>
double neuralNetwork<T>::validate(double * imgs, unsigned char * labels)
{
    int nbatch = 10000/BatchSize;
    double * batch = (double*) malloc(sizeof(double)*BatchSize*InputDim);
    double * label = (double*) malloc(sizeof(double)*BatchSize);
    double error = 0 ;
    for(int b=0;b<nbatch;b++)
    {
    int start=b*BatchSize;
    for(int i=0;i<BatchSize;i++)
        for(int j=0;j<InputDim;j++)
            batch[i + BatchSize*j] = imgs[start*InputDim + i*InputDim + j];

    for(int i=0;i<BatchSize;i++)
        label[i]= labels[start+i];
    
    //start forward propagate
    cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,
    NodesPerLayer,BatchSize,InputDim,
    1,W1,InputDim,batch,BatchSize,0,Z[0],BatchSize);
    for(int i=0;i<NodesPerLayer*BatchSize;i++)
        {
            int j =i/BatchSize;
            Z[0][i] += Bhidden[0][j];
            A[0][i] = Relu(Z[0][i]);
        }
    
    for(int i=1;i<=NHiddenLayers-1;i++)
        {
        cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans, NodesPerLayer,
        BatchSize,NodesPerLayer,1,Whidden[i-1],NodesPerLayer,A[i-1],BatchSize,
        0,Z[i],BatchSize);
        for(int j=0;j<NodesPerLayer*BatchSize;j++)
            {
                Z[i][j] = Z[i][j] + Bhidden[i][j];
                A[i][j] = Relu(Z[i][j]);
            }
        }
        cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans, OutNodes,BatchSize,
        NodesPerLayer,1,Wout,NodesPerLayer,A[NHiddenLayers-1],BatchSize,0,
        Zout,BatchSize);
        
        for(int i=0;i<OutNodes*BatchSize;i++)
            {int j = i/BatchSize;
            out[i] = Zout[i] + Bout[j];
            }
        softmax(out,OutNodes,BatchSize);
        for(int i=0;i<BatchSize;i++)
            error+=(-log(out[int(label[i])*BatchSize + i]));
}return error;
}
