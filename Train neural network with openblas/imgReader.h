#include <stdio.h>
#include <stdlib.h>
class imgReader{
public:
char magic[4];
int num_images;
int num_rows;
int num_cols;    
void convert(){
    convertEndianess(&num_images);
    convertEndianess(&num_rows);
    convertEndianess(&num_cols);
    printf("magic:%02x%02x%02x%02x\n",magic[0],magic[1],magic[2],magic[3]);
    printf("numberImages: %i, numRows: %i, numCols:%i\n",num_images,num_rows,num_cols);
}
void convertEndianess(int *p){
char * temp;
temp= (char*)p;
int t1,t2,t3,t4;
t1 = *(temp+3);
t2 = *(temp+2);
t3 = *(temp+1);
t4 = *(temp);
*(temp)=t1;
*(temp+1)=t2;
*(temp+2)=t3;
*(temp+3)=t4;
return;
}
};