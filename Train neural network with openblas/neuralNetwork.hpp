# ifndef neuralNetwork_H
#   define neuralNetwork_H
#include <string>
#include <memory>
#include <stdlib.h>
#include "cblas.h"
template<class T>
class neuralNetwork
{
public:
neuralNetwork(size_t nhidden, size_t nodeperlayer,size_t outNodes,size_t inputDim,size_t batchSize):
NHiddenLayers(nhidden),NodesPerLayer(nodeperlayer),OutNodes(outNodes),InputDim(inputDim),BatchSize(batchSize)
{
//Kaiming initialization

srand(time(NULL));
double stddev = 2.0d/InputDim;
W1=(T*)malloc(sizeof(T)*NodesPerLayer*inputDim);
uW1=(T*)malloc(sizeof(T)*NodesPerLayer*inputDim);

for(int i=0;i<NodesPerLayer*inputDim;i++)
    {
        double j = (double)rand()/(double)RAND_MAX;
        W1[i] = (j-0.5)*stddev;
    } 

 Wout=(T*)malloc(outNodes*nodeperlayer*sizeof(T));
uWout=(T*)malloc(outNodes*nodeperlayer*sizeof(T));

stddev = 2.0d/OutNodes;
for(int i=0;i<outNodes*nodeperlayer;i++)
    {
        double j = (double)rand()/(double)RAND_MAX;
        Wout[i] = (j-0.5)*stddev;
    } 

Bout=(T*)calloc(outNodes,sizeof(T));

stddev = 2.0d/NodesPerLayer;
tempW=(T*)malloc(sizeof(T)*nodeperlayer*nodeperlayer*(nhidden-1));
utempW=(T*)malloc(sizeof(T)*nodeperlayer*nodeperlayer*(nhidden-1));

for(int i=0;i<nodeperlayer*nodeperlayer*(nhidden-1);i++)
    {
        double j = (double)rand()/(double)RAND_MAX;
        tempW[i] = (j-0.5)*stddev;
    }

tempB=(T*)calloc(nodeperlayer*nhidden,sizeof(T));

for(int i=0;i<nodeperlayer*nhidden;i++)
    tempB[i] = (((double)rand()/(double)RAND_MAX)-0.5)*stddev;
        

Whidden=(T**)malloc(sizeof(T*)*(nhidden-1));
uWhidden=(T**)malloc(sizeof(T*)*(nhidden-1));
Bhidden=(T**)malloc(sizeof(T*)*(NHiddenLayers));

for(int i=0;i<nhidden-1;i++)
    {
        Whidden[i]=&tempW[nodeperlayer*nodeperlayer*i];
        uWhidden[i] = &utempW[nodeperlayer*nodeperlayer*i];
    }

for(int i=0;i<nhidden;i++)
    Bhidden[i]= &tempB[nodeperlayer*i];
    
A=(T**)malloc(sizeof(T*) * nhidden);
Z=(T**)malloc(sizeof(T*) * nhidden);
deltal=(T**)malloc(sizeof(T*)*(nhidden+1));
deltal[nhidden] = (T*)calloc(outNodes*BatchSize,sizeof(T));
deltal1d = (T**)malloc((nhidden+1)*sizeof(T*));
deltal1d[nhidden] = (T*)calloc(outNodes,sizeof(T));

for(int i=0;i<nhidden;i++)
    {
        A[i] = (T*)calloc(nodeperlayer*BatchSize,sizeof(T));
        Z[i] = (T*)calloc(nodeperlayer*BatchSize,sizeof(T));
        deltal[i] = (T*)malloc(sizeof(T)*nodeperlayer*BatchSize);
        deltal1d[i] = (T*)calloc(NodesPerLayer,sizeof(T));
    }

out = (T*)malloc(sizeof(T)*outNodes*BatchSize);
Zout = (T*)malloc(sizeof(T)*outNodes*BatchSize);
}

//~neuralNetwork()
//{}

T sigmoid(T z);
void train_CPU(double* img,double* labelVecs,int numTraing, unsigned char* labels, T alpha,int niters);
void train_GPU(double* img, double* labelVector,int numTrain, unsigned char* labelsforval,T alpha,int niters);
void forward(unsigned char* img,size_t numTraing);
int predict(double * imgs);
double validate(double *,unsigned char* labels);
size_t NHiddenLayers;
size_t NodesPerLayer;
size_t OutNodes;
size_t InputDim;
size_t BatchSize;
T* tempW;
T* utempW;
T* W1;
T* uW1;
T** Whidden;
T** uWhidden;
T* Wout;
T* uWout;
T* utempB;
T* tempB;
T** Bhidden;
T* Bout;
T** A;
T** Z;
T* Zout;
T* out;
T** deltal;
T** deltal1d;
};
#endif