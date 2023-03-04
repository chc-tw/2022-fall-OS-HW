#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int A[800][800];
int ystart[17];
int dim;

void matma(int ystart, int yend, int* sh_mem);
void forking(int num, int* sh_mem);

int main(){
    int segment_id;
    scanf("%d",&dim);
    int init = 0;
    struct timeval start,end;
    for(int i = 0; i<dim ;i++)
        for(int k = 0; k<dim ;k++)
            A[i][k] = init++;

    int num = 1;
    int checksum = 0;
    int flag = 1;
    segment_id = shmget (IPC_PRIVATE, (dim*dim)*sizeof(int),
                        IPC_CREAT | 0600);

    int* sh_mem = (int *) shmat(segment_id, 0, 0);
    int* temp =sh_mem;
    while(num<=16){
        int interval = dim/num;
        memset(ystart,'\0',num+1);
        memset(sh_mem,0,dim*dim);
        for(int i=0;i<=num;i++){
            ystart[i] = interval * i;
            if(i==num)
                ystart[i] = dim ;
            //printf("ystart[%d]:%d\n",i,ystart[i]);
        }
        gettimeofday(&start,0);
        forking(num,sh_mem);
        gettimeofday(&end,0);
        int sec = end.tv_sec - start.tv_sec;
        int usec = end.tv_usec - start.tv_usec;
        for(int i = 0; i<dim ;i++)
            for(int k = 0; k<dim ;k++){
                //printf("%d ", *sh_mem);
                checksum += *sh_mem;
                sh_mem++;
            }
        printf("Mutiplying matrices using %d processes\n",num);
        printf("Elapsed time: %f sec",sec+(usec/1000000.0));
        printf(", Checksum: %d\n",checksum);
        checksum = 0;     
        num++;
        sh_mem = temp;
    }
    
    shmdt(sh_mem);
    shmctl(segment_id, IPC_RMID, 0);
}

void matma(int ystart, int yend, int* sh_mem){
    //printf("%p\n",sh_mem);
    int index = ystart * dim;
    sh_mem += index;
    //printf("ystart: %d yend: %d\n",ystart,yend);
    for(int i=ystart;i<yend;i++){
        for (int j=0;j<dim;j++){
            int sum = 0;
            for (int k=0;k<dim;k++){
                sum = sum + A[i][k]*A[k][j];
            }
            *sh_mem = sum;
            sh_mem++;
            //printf("%d ",sum);
        }
        //printf("\n");
    }
}

void forking(int num, int* sh_mem){
    if(num>0){
        num--;
        pid_t pid;
        pid = fork();
        if (pid < 0){ //child fork fail
            printf("\n");
            fprintf(stderr,"Fork Failed");
            exit(-1);   
        }
        else if (pid == 0){ //child fork
            
            forking(num,sh_mem);
            exit(0);
            //printf("child %d\n",num);
        }
        else { //parent fork;
            matma(ystart[num],ystart[num+1],sh_mem);
            wait(NULL);
            //printf("parent %d\n",num);
        }
    }
    
}
