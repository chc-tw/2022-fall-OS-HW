#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <vector>
#include <utility>
#include <iostream>
#include <functional>
#include <queue>
#include <unistd.h>
#include <algorithm>
#include <time.h>
#include <sys/time.h>
using namespace std;

int Max = INT_MAX;

void printSub(vector< vector<int>> subArr);
//void bubblesort(vector<int> &subArr);
//vector<int> merge(vector<int> subArr1, vector<int> subArr2);
void* bubblesort(void *arg);
void* merge(void *arg);
typedef struct mergePair{
    int index;
    vector<int> subArr1;
    vector<int> subArr2;
    vector<int> result;
    bool check = false;
}mergePair;
typedef struct sortArr{
    int index;
    vector<int> subArr;
    bool check = false;
}sortArr;
class dispatcher{
public:
    pthread_attr_t attr;
    vector <pthread_t> pool;
    vector <bool> poolBusy;
    int eleNum;
    int threadNum;
    vector < vector<int> > sortQueue;
    vector < vector<int> > mergeQueue;
    vector <mergePair*> mergeCheck;
    vector <sortArr*> sortCheck;
    int mergePairNum = 0;
    vector <int> taskQueue; // 0 for sort and 1 for merge
    sem_t queueMutex[4]; //0: sort, 1:merge, 2:task, 3:poolbusy
    sem_t counter; //
    void fetchTask(){
            //check type of work
            sem_wait(&queueMutex[2]);
            int taksIndex = taskQueue.front();
            taskQueue.erase(taskQueue.begin());
            sem_post(&queueMutex[2]);
            int threadIndex;
            //find
            sem_wait(&queueMutex[3]);
            for(int i=0;i<threadNum;i++){
                if(!poolBusy[i]){
                    threadIndex = i;
                    poolBusy[i] = true;
                    break;
                }
            }
            sem_post(&queueMutex[3]);
            if(taksIndex==0){
                //printf("fetchsort\n");
                sem_wait(&queueMutex[0]);
                sortArr *temp = new sortArr();
                temp->subArr = sortQueue[0];
                temp->index = threadIndex;
                sortCheck.push_back(temp);
                sortQueue.erase(sortQueue.begin());
                //pthread_detach(pool[threadIndex]);
                int err = pthread_create(&pool[threadIndex],&attr,bubblesort,(void*) temp);
                if(0!=err) cout<<"error:"<<err<<endl;
                
                sem_post(&queueMutex[0]);
                //printf("fetchsort done\n");
            }
            else if(taksIndex==1){
                //printf("fetchmerge\n");
                sem_wait(&queueMutex[1]);
                mergePair *temp = new mergePair();
                temp->subArr1 = mergeQueue[0];
                temp->subArr2 = mergeQueue[1]; 
                temp->index = threadIndex;
                mergeQueue.erase(mergeQueue.begin());
                mergeQueue.erase(mergeQueue.begin());
                mergeCheck.push_back(temp);
                //pthread_detach(pool[threadIndex]);
                int err = pthread_create(&pool[threadIndex],&attr,merge,(void*) temp);
                if(0!=err) cout<<"error:"<<err<<endl;
                sem_post(&queueMutex[1]);
                //printf("fetchmerge done\n");
            }
            else{
                printf("getTask error\n");
            }
        }
    
    dispatcher(int threadNum_, int eleNum_,vector< vector<int> > subArr){
        threadNum = threadNum_;
        eleNum = eleNum_;
        //initialize semaphores
        sem_init(&counter,1,threadNum_);
        for(int i=0; i<4;i++)
            sem_init(&queueMutex[i],1,1);
        pool.resize(threadNum);
        poolBusy.resize(threadNum);

        //intialize thread situation
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
        for(int i=0;i<poolBusy.size();i++)
            poolBusy[i] = false;

        //initiailize queue
        for(int i=0; i<8; i++){
            sortQueue.push_back(subArr[i]);
            sem_wait(&queueMutex[2]);
            taskQueue.push_back(0);
            sem_post(&queueMutex[2]);
        }
        //ceate n threads with sorting task
        for(int i=0; i<threadNum; i++){
            sem_wait(&counter);
            fetchTask();
            //printf("test%d\n",i);
        }
        //printf("initialize done");
    }
    
    void checkMerg(){
        if(mergePairNum>=2){
            sem_wait(&queueMutex[2]);
            taskQueue.insert(taskQueue.begin(),1);
            sem_post(&queueMutex[2]);
            mergePairNum -= 2;
        }
    }
    void checkTask(){
        // check merge
        sem_wait(&queueMutex[1]);
        //cout<<"mergeCheck size:"<<mergeCheck.size()<<endl;
        for(int i=0; i<mergeCheck.size(); i++){
            if(mergeCheck[i]->check){
                mergeQueue.push_back(mergeCheck[i]->result);
                mergePairNum++;
                //mergeCheck.erase(mergeCheck.begin()+i);
                poolBusy[mergeCheck[i]->index] = false;
                mergeCheck[i] = nullptr;
                sem_post(&counter);
            }
        }
        auto iter1 = remove(mergeCheck.begin(), mergeCheck.end(), nullptr);
        mergeCheck.erase(iter1, mergeCheck.end());
        sem_post(&queueMutex[1]);
        // check sort
        sem_wait(&queueMutex[0]);
        //cout<<"sortCheck size:"<<sortCheck.size()<<endl;
        for(int i=0; i<sortCheck.size(); i++){
            //for(auto it =sortCheck[i]->subArr.begin(); it!= sortCheck[i]->subArr.end();it++)
            //    cout<<*it<<" ";
            //cout<<"\n";
            if(sortCheck[i]->check){
                //cout<<"this sort subarr size:"<<sortCheck[i]->subArr.size()<<endl;
                sem_wait(&queueMutex[1]);
                mergeQueue.push_back(sortCheck[i]->subArr);
                mergePairNum++;
                sem_post(&queueMutex[1]);
                //sortCheck.erase(sortCheck.begin()+i);
                poolBusy[sortCheck[i]->index] = false;
                sortCheck[i] = nullptr;
                sem_post(&counter);
            }
        }
        auto iter2 = remove(sortCheck.begin(), sortCheck.end(), nullptr);
        sortCheck.erase(iter2,sortCheck.end());
        sem_post(&queueMutex[0]);
    }
    bool done(){
        //cout<<mergeCheck.size()<<" "<<taskQueue.size()<<" "<<mergeQueue[1].size()<<endl;
        if(mergeCheck.size()==0 && taskQueue.size()==0 && !mergeQueue.empty())
            if(mergeQueue[0].size()==eleNum)
                return false;
        return true;
    }
};

int main(){
    //read file
    FILE *f1 = fopen("input.txt", "r");
    int temp;
    vector<int> arr;
    int arrNum;
    fscanf(f1,"%d", &arrNum);
    while(fscanf(f1,"%d", &temp) != EOF)
        arr.push_back(temp);
    fclose(f1);
    //cout<<arrNum<<endl;
    // divide array into 8 subarrays
    vector< vector<int> > subArr;
    int eleNum = arr.size()/8;
    int begin;
    // The main program
    int n = 1;
    while(n<=8){
        begin = 0;
        subArr.clear();
        for(int i=0 ; i<8; i++){
            vector<int> temp;
            if(i<=6)
                temp.assign(arr.begin()+begin,arr.begin()+begin+eleNum);
            else
                temp.assign(arr.begin()+begin,arr.end());
            subArr.push_back(temp);
            begin += eleNum;
        }
        struct timeval start,end;
        char filename[20] = "output_0.txt";
        filename[7] = n + '0';
        FILE *f2 = fopen(filename,"a");
        gettimeofday(&start,0);
        dispatcher mergeSort(n,arrNum, subArr);
        int curNum;
        //printSub(subArr);
        //cout<<"done:"<<mergeSort.done()<<endl;
        sem_getvalue(&mergeSort.counter,&curNum);
        //printf("outside,counter:%d",curNum);
        while(mergeSort.done()){
            mergeSort.checkTask();
            mergeSort.checkMerg();
            sem_getvalue(&mergeSort.counter,&curNum);
            //printf("counter:%d\n",curNum);
            if(mergeSort.taskQueue.size()!=0 && curNum !=0){
                sem_wait(&mergeSort.counter);
                mergeSort.fetchTask();
            }
        }
        //printSub(mergeSort.mergeQueue);
        gettimeofday(&end,0);
        for(auto it = mergeSort.mergeQueue[0].begin();it!=mergeSort.mergeQueue[0].end();it++)
            if(it+1 == mergeSort.mergeQueue[0].end())
                fprintf(f2,"%d",*it);
            else
                fprintf(f2,"%d ",*it);
            
        int sec = end.tv_sec - start.tv_sec;
        int usec = end.tv_usec - start.tv_usec;
        printf("worker thread #%d, elapsed %f ms\n",n,(sec*1000.0)+(usec/1000.0));
        n++;
    }
}
void printSub(vector< vector<int>> subArr){
    for (int i=0; i<subArr.size();i++){
        for(auto it = subArr[i].begin(); it!=subArr[i].end(); it++)
            cout<<*it<<" ";
        cout<<"\n";
    }
}

void *bubblesort(void *arg){
    //printf("bubble\n");
    sortArr *Arr = (sortArr *) arg;
    int temp;
    for(int i=(Arr->subArr.size()-1) ; i>0; i--){
        for(int k = 0; k < i; k++){
            if(Arr->subArr[k+1] < Arr->subArr[k]){
                temp = Arr->subArr[k];
                Arr->subArr[k] = Arr->subArr[k+1];
                Arr->subArr[k+1] = temp;
            }
        }
    }
    Arr->check=true;
    //printf("bubble done\n");
    //pthread_exit(NULL);
}

void* merge(void *arg){
    //printf("merge\n");
    mergePair *arr_ = (mergePair *)arg;
    vector<int> subArr1 = arr_->subArr1;
    subArr1.push_back(Max);    
    vector<int> subArr2 = arr_->subArr2;
    subArr2.push_back(Max);
    vector<int> arr;
    //int begin = subArr1.second < subArr2.second ? subArr1.second : subArr2.second;
    int idxLeft = 0, idxRight = 0;
    for (int i = 0; i < subArr1.size()-1 + subArr2.size()-1; i++) {
        if (subArr1[idxLeft] <= subArr2[idxRight]) {
            arr.push_back(subArr1[idxLeft]);
            idxLeft++;
        }
        else{
            arr.push_back(subArr2[idxRight]);
            idxRight++;
        }
    }
    arr_->result = arr;
    arr_->check = true;
    //printf("merge done\n");
}


/*void bubblesort(vector<int> &subArr){
    for(int i=(subArr.size()-1) ; i>0; i--)
        for(int k = 0; k < i; k++)
            if(subArr[k+1] < subArr[k]){
                int temp = subArr[k];
                subArr[k] = subArr[k+1];
                subArr[k+1] = temp;
            }
}
vector<int> merge(vector<int> subArr1, vector<int> subArr2){
    vector<int> arr;
    subArr1.push_back(Max);
    subArr2.push_back(Max);
    //int begin = subArr1.second < subArr2.second ? subArr1.second : subArr2.second;
    int idxLeft = 0, idxRight = 0;
    for (int i = 0; i < subArr1.size()-1 + subArr2.size()-1; i++) {
        if (subArr1[idxLeft] <= subArr2[idxRight]) {
            arr.push_back(subArr1[idxLeft]);
            idxLeft++;
        }
        else{
            arr.push_back(subArr2[idxRight]);
            idxRight++;
        }
    }
    return arr;
}*/
