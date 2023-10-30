#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE (1024*32)
#define BPS 10 //bits per symbol
int arr[SIZE];
int read_arr[SIZE];
int main() {
    
    //int read_arr[SIZE];
    int range=100;
    int offset=0;
    int rand_num;
    FILE *fp;
    fp = fopen("random_ampl_30000_10BPS.bin", "wb");
    if (fp == NULL) {
        printf("Error opening file.");
        return 1;
    }

    // Generate random numbers and write to file
    int sgn=1;
    int ampl=30000;
    arr[0]=ampl;
    //start bits
    for(int i=1;i<800;i++)
    {
        if(i%20==0)
            sgn=sgn*(-1);
        arr[i]=sgn*ampl;
    }
    for(int i=800;i<1000;i++)
    {
        arr[i]=0;
    }
    srand(time(NULL));
    //generating numbers which are either +ampl or -ampl
    for (int i = 1000; i < SIZE; i+=BPS) 
    {
        rand_num = (rand()%range)+offset;
        if(rand_num>=50)
            rand_num=1;
        else
            rand_num=-1;
        for(int j = 0;j<BPS;j++)
        {
            arr[i+j]=rand_num*ampl;
        }
        
        
    }
    
    for(int i=0;i<SIZE;i++)
    {
        fwrite(&arr[i], sizeof(int), 1, fp);
    }
    fclose(fp);

    // Read numbers from file and add to array
    //verifying if the numbers are generated correctly
    
    fp = fopen("random_ampl_30000_10BPS.bin", "rb");
    if (fp == NULL) {
        printf("Error opening file.");
        return 1;
    }
    for (int i = 0; i < SIZE; i++) {
        fread(&read_arr[i], sizeof(int), 1, fp);
    }
    fclose(fp);

    // Print array values
    for (int i = 0; i < 2000; i++) {
        printf("%d ", arr[i]);
    }
    
    return 0;
}
