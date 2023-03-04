/*
Student No.:10811017
Student Name:張皓丞
Email: sam24341094.be08@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am full aware that this program is not
supposed to be posted to a public server, such as a 
public GitHub repository or a public web page.
*/
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
const char* test ="&";
int main(){
    pid_t pid;
    char ter[1000];
    char *temp;
    char *parameter[1000];
    while(1){
        int pip[2];
        pipe(pip); //0 is read 1 is write
        int i = 0;
        usleep(3000*1);
        printf(">");

        // get the cmd

        fgets(ter,1000,stdin);
        temp = strtok(ter," \n");
        while(temp != NULL){
            parameter[i++] = strdup(temp);
            temp = strtok(NULL," \n");
        }
        //cmd = strdup(parameter[0]);
        parameter[i] = NULL;
        //for (int k = 0; parameter[k]!=NULL;k++) printf("%d:%s\n",k,parameter[k]);
        //printf("i: %d\n",i);
        //core code

        pid = fork();
        if (pid < 0){ //child fork fail
            printf("1\n");
            fprintf(stderr,"Fork Failed");
            exit(-1);   
        }

        else if (pid == 0){ //child fork
            if (strcmp(parameter[i-1], "&") ==0){ //check whether sleep
                parameter[i-1] = NULL; //don't need to wait
                pid_t pid2;
                pid2 = fork();
                if (pid2 < 0){ //grandchild fork fail
                    fprintf(stderr,"Fork2 Failed");
                    exit(-1);   
                }
                else if (pid2 == 0){ // grandchild fork
                    if(execvp(parameter[0],parameter)<0)
                    printf("error in execvp\n ");
                }
                else { //child fork
                    exit(0);
                }
            }
            else{ // need to wait
                int flag = 1;
                for(int j = 0; j <= i-1; j++){
                    if(strcmp(parameter[j],">") == 0){ // check >
                        int fd = open(parameter[i-1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                        parameter[i-2] = parameter[i];
                        dup2(fd, 1);
                        flag = 0;
                        if(execvp(parameter[0],parameter)<0)
                            printf("error in execvp"); 
                        close(fd);
                        break;
                    }
                    if(strcmp(parameter[j],"<") == 0){ // check <
                        int fd = open(parameter[i-1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                        parameter[i-2] = parameter[i];
                        dup2(fd, 0);
                        flag = 0;
                        if(execvp(parameter[0],parameter)<0)
                            printf("error in execvp"); 
                        close(fd);
                        break;
                    }
                    if(strcmp(parameter[j],"|") == 0){ //check |
                        char* parameter2[100];
                        int t = 0;
                        for(int k = j+1 ; k <= i ; k++)
                            parameter2[t++] = parameter[k];
                        parameter[j] = NULL;

                        pid_t pid2;
                        pid2 = fork();

                        if (pid2 < 0){ //grandchild fork fail
                            printf("1\n");
                            fprintf(stderr,"Fork Failed");
                            exit(-1);   
                        }

                        else if (pid2 == 0){ // grandchild fork
                            /*close(pip[1]);
                            dup2(pip[0], STDIN_FILENO);
                            close(pip[0]); */
                            close(1);          //closing stdout
                            dup2(pip[1],1);     //replacing stdout with pipe write 
                            close(pip[0]);   //closing pipe read
                            close(pip[1]);
                            //dup2(pip[1], 1);
                            if(execvp(parameter[0],parameter)<0)
                            printf("error in execvp"); 
                            exit(0);
                        }

                        else{ //child fork
                            /*close(pip[0]);  
                            dup2(pip[1],STDOUT_FILENO);         
                            close(pip[1]);*/
                            close(0);        //closing stdin
                            dup2(pip[0],0);   //replacing stdin with pipe read
                            close(pip[1]); //closing pipe write
                            close(pip[0]);
                            if(execvp(parameter2[0],parameter2)<0)
                            printf("error in execvp"); 
                        }
                        flag = 0;
                        break;
                    }
                }
                if(flag) 
                    if(execvp(parameter[0],parameter)<0)
                        printf("error in execvp"); 
            }
        }
        else { //parent fork
            //if (strcmp(parameter[i-1], "&") !=0) wait(NULL);
            close(pip[0]);
            close(pip[1]);
            wait(NULL); 
        }
    }
    /*fork another process*/
}