#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <math.h>
#include<iostream>
#include "imgReader.h"
#include "neuralNetwork.hpp"
#define max(a, b) ((a) > (b))? (a):(b)
#define TRAIN_IMAGE "./data/train-images-idx3-ubyte"
#define TRAIN_LABEL "./data/train-labels-idx1-ubyte"
#define TEST_IMAGE  "./data/t10k-images-idx3-ubyte"
#define TEST_LABEL  "./data/t10k-labels-idx1-ubyte"
#define NUM_LABEL 10
using namespace std;
void convertEndianess(int *p){
char * temp;
temp=(char*)p;
int t1,t2,t3,t4;
t1 = *(temp+3);
t2 = *(temp+2);
t3 = *(temp+1);
t4 = *(temp);
*(temp)=t1;
*(temp+1)=t2;
*(temp+2)=t3;
*(temp+3)=t4;
return ;
}

int main()
{
srand(time(NULL));
FILE *file = fopen(TRAIN_IMAGE, "rb");
 if (file == NULL) {
    printf("Error opening file!\n");
    return 1;
  }
FILE *TrainLabeFile = fopen(TRAIN_LABEL,"rb");
 if (TrainLabeFile == NULL) {
    printf("Error opening file!\n");
    return 1;
  }
imgReader h1;
imgReader h2;
fread(&h1,sizeof(h1),1,file);
fread(&h2,8,1,TrainLabeFile);
h1.convert();
h2.convert();
unsigned char*imgs=(unsigned char*)malloc(h1.num_cols*h1.num_rows*h1.num_images);
unsigned char*labels = (unsigned char*)malloc(h2.num_images);
fread(imgs,sizeof(unsigned char),h1.num_cols*h1.num_rows*h1.num_images,file);
fread(labels,sizeof(unsigned char), h2.num_images,TrainLabeFile);
double *labelVector = (double*)malloc(sizeof(double)*NUM_LABEL*h1.num_images);
for(int i=0;i<h1.num_images;i++)
    for(int j=0;j<NUM_LABEL;j++)
        if(j==int(labels[i]))
            labelVector[i+j*h1.num_images]=1;
        else
            labelVector[i+j*h1.num_images]=0;
double *normalImg = (double*)malloc(h1.num_cols*h1.num_rows*h1.num_images*sizeof(double));

for(int i=0;i<h1.num_cols*h1.num_rows*h1.num_images;i++)
    normalImg[i] = double(imgs[i]/255.0);


neuralNetwork<double> n1(1,800,10,784,200);
n1.train_CPU(normalImg,labelVector,h1.num_images-10000,labels,0.1,30);


double *IMGS =(double*)malloc(sizeof(double)*784);
for(int k=58000;k<58020;k++)
{for(int i=k*784;i<k*784+784;i++)
    IMGS[i-k*784]=normalImg[i];
printf("label: %u, predict: %d\n", labels[k], n1.predict(IMGS));
}
}