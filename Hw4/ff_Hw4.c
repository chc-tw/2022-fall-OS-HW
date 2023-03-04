#ifndef __FF_H__
#define __FF_H__
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
typedef struct block block;
struct block {
    size_t size;
    int free;
    struct block *prev;
    struct block *next;
};
char* intostr(int arc){
    char* temp;
    char ttemp[5];
    int rev;
    int i = -1;
    int num = arc;
    while(num!=0){
        i++;
        num/=10;
    }
    ttemp[i+1] = '\n';
    for(; i>=0; i-- ){
        ttemp[i] = arc%10 + '0';
        arc /= 10;
    }
    temp = ttemp;
    return temp;
}
block *pool = NULL, *end = NULL;
void *malloc(size_t size){
    int dataNum = size%32==0 ? size/32 : size/32 +1;
    if (pool == NULL){ //the first malloc to initialize the pool
        pool = mmap(NULL,20000,PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1,0);
        end = pool + 625;
        block *chunk1 = pool;
        block *chunk2 = pool + dataNum + 1;  
        block temp1 = {
            .size = dataNum*32 + 32,
            .free = 0,
            .prev = NULL,
            .next = chunk2
        };
        block temp2 = {
            .size = 20000 - temp1.size,
            .free = 1,
            .prev = chunk1,
            .next = NULL
        };
        if(sizeof(temp1)!= 32){ //to print error of block size being not 64-bit
            char *err = "The computer is 32-bit instead of 64-bit so the answer cannot be displayed correctly";
            write(1,err,strlen(err));
        }
        memcpy(chunk1,&temp1,sizeof(temp1));    
        memcpy(chunk2,&temp2,sizeof(temp2));
        return (void*)(chunk1 + 1);
    }
    else if (size == 0){// print result
        block* tptr = pool;
        int max = 0;
        while(tptr!=NULL){
            if(tptr->free && tptr->size > max) max = tptr->size;
            tptr = tptr->next;
        }
        max -= 32;
        
        char* out1 = "Max Free chunk Size = ";
        write(1,out1,strlen(out1));
        char* out2 = intostr(max);
        write(1,out2,strlen(out2));
        munmap(pool,20000);
        return NULL;
    }
    else {
        block* tptr = pool;
        //first fit part
        while(tptr->size < size + 32 || !tptr->free){
            tptr = tptr->next;
        }
        int curNum = size % 32 == 0 ? size/32 : size/32 +1;
        block* chunk = tptr + curNum + 1;
        block temp = {
            .size = tptr->size - curNum * 32 - 32,
            .free = 1,
            .prev = tptr,  
            .next = tptr->next
        };
        tptr->size = 32 + curNum * 32;
        tptr->free = 0;
        if(tptr->next != chunk){
            tptr->next = chunk;
            memcpy(chunk,&temp,sizeof(temp));
        }
        return (void*)( tptr + 1);
    }
}
void free( void *ptr){
    block *tptr = (block *)ptr;
    tptr -= 1;
    tptr->free = 1;
    //check left
    if(tptr->prev != NULL ){
        if(tptr->prev->free){
            int totSize = tptr->prev->size + tptr->size;
            tptr->prev->next = tptr->next;
            tptr->next->prev = tptr->prev;
            tptr->prev->size = totSize;
        }
    }
    //check right
    if(tptr->next !=NULL){
        if(tptr->next->free){
            int totSize = tptr->next->size + tptr->size;
            tptr->next->next->prev = tptr;
            tptr->next = tptr->next->next;
            tptr->size = totSize;       
        }
    }
}
#endif


