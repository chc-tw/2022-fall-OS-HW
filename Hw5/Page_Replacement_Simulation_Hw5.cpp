#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <vector>
#include <unordered_map>
#include <utility>
#include <unistd.h>
#include <list>
using namespace std;

typedef vector<pair<int, int> > vw; // first is the page ref, second is the count
typedef unordered_map<int, pair<int,int> > fl; // first is the page ref, second is index in vw
int frameNum,hit, miss,refNum;
void swap(pair<int, int>& a, pair<int, int>& b);
int parent(int i);
int left(int i);
int right(int i);
void swim(vw& freq,fl& pages, int i, int n);
void sink(vw& freq,fl& pages, int i, int n);
void LFUaccess(vw &freq,fl &pages,int ref, int &size);
void insert(vw &freq,fl &pages,int ref, int &size);
void add(vw &freq,fl &pages,int ref, int &size);
void LRUaccess(list<int> &rec, unordered_map<int,list<int>::iterator> &pages, int ref);

int main(int argc, char **argv){
    char* filename = argv[1];
    // LFU
    int n = 4;
    struct timeval start,end;
    vw freq;
    fl pages;
    printf("LFU policy:\n");
    printf("Frame\tHit\tMiss\tPage fault ratio\n");
    gettimeofday(&start,0);
    while(n--){  
        switch (n)
        {
        case 3:
            frameNum = 64;
            break;
        case 2:
            frameNum = 128;
            break;
        case 1:
            frameNum = 256;
            break;
        case 0:
            frameNum = 512;
            break;
        default:
            break;
        }
        FILE *fp = fopen(filename,"r");
        int num,size =0;
        //freq.resize(frameNum);
        refNum = 1;
        freq.clear();
        pages.clear();
        while(fscanf(fp,"%d",&num) != EOF){
            LFUaccess(freq, pages, num, size);
            refNum++;
        }
        printf("%d\t%d\t%d\t%.10f\n",frameNum,hit,miss,(double)miss/(double)(hit+miss));
        hit = 0;
        miss = 0;
        fclose(fp);
    }
    gettimeofday(&end,0);
    int sec = end.tv_sec - start.tv_sec;
    int usec = end.tv_usec - start.tv_usec;
    printf("Total elapsed time %f sec\n",sec+(usec/1000000.0));

    // LRU
    n = 4;
    printf("LRU policy:\n");
    printf("Frame\tHit\tMiss\tPage fault ratio\n");
    gettimeofday(&start,0);
    list<int> rec;
    unordered_map <int,list<int>::iterator> LRUpages;
    while(n--){
        switch (n)
        {
        case 3:
            frameNum = 64;
            break;
        case 2:
            frameNum = 128;
            break;
        case 1:
            frameNum = 256;
            break;
        case 0:
            frameNum = 512;
            break;
        default:
            break;
        }
        FILE *fp = fopen(filename,"r");
        int num;
        LRUpages.clear();
        rec.clear();
        while(fscanf(fp,"%d",&num) != EOF){
            LRUaccess(rec, LRUpages, num);
        }
        printf("%d\t%d\t%d\t%.10f\n",frameNum,hit,miss,(double)miss/(double)(hit+miss));
        hit = 0;
        miss = 0;
        fclose(fp);
    }
    gettimeofday(&end,0);
    sec = end.tv_sec - start.tv_sec;
    usec = end.tv_usec - start.tv_usec;
    printf("Total elapsed time %f sec\n",sec+(usec/1000000.0));
}
void swap(pair<int, int>& a, pair<int, int>& b){
    pair<int, int> temp = a;
    a = b;
    b = temp;
}
void swim(vw& freq,fl& pages, int i, int size){
    while(i>0 && freq[i].second < freq[parent(i)].second){
        pages[freq[i].first].first = parent(i);
        pages[freq[parent(i)].first].first = i;
        swap(freq[i],freq[parent(i)]);
        i = parent(i);
    }
}
void sink(vw& freq,fl& pages, int i, int size){
    int l = left(i), r = right(i);
    if(l < size){
        int j = l;
        
        if(r < size && freq[r].second < freq[l].second  ){
            j = r;
        }
        else if (r < size && freq[r].second == freq[l].second){
            if(pages[freq[r].first].second < pages[freq[l].first].second){
                j = r;
            }
        }
        if(freq[i].second>=freq[j].second){
            pages[freq[i].first].first = j;
            pages[freq[j].first].first = i;
            swap(freq[i],freq[j]);
            sink(freq,pages,j,size);
        }
        
    }
}
void LFUaccess(vw &freq,fl &pages,int ref, int &size){
    if(pages.count(ref)){
        hit++;
        add(freq, pages,ref,size);
    }
    else{
        miss++;
        insert(freq, pages,ref,size);
    }
}
void insert(vw &freq,fl &pages,int ref, int &size){
    if(freq.size()==frameNum){
        pages.erase(freq[0].first); // It's like extract max
        freq[0] = freq[--size];
        freq.erase(freq.end());
        sink(freq,pages,0,size);
    }
    //freq[size++]=make_pair(ref,1);
    
    freq.push_back({ref,1});
    size++;
    pages.insert({ref,{size-1,refNum}});
    swim(freq,pages,size-1,size);
}
void add(vw &freq,fl &pages,int ref, int &size){
    //if(k<100)
        //printf("add %d:%d\n",freq[pages[ref]].first,freq[pages[ref]].second);
    freq[pages[ref].first].second++;
    pages[ref].second = refNum;
    //if(k<100)
        //printf("after add %d:%d\n",freq[pages[ref]].first,freq[pages[ref]].second);
    sink(freq,pages,pages[ref].first,size);
}
int parent(int i){
    return (i - 1) / 2;
}
int left(int i){
    return 2 * i + 1;
}
int right(int i){
    return 2 * i + 2;
}
void LRUaccess(list<int> &rec, unordered_map<int,list<int>::iterator> &pages, int ref){
    if(!pages.count(ref)){
        miss++;
        if(rec.size() == frameNum){
            int temp = rec.back();
            rec.pop_back();
            pages.erase(temp);
        }
    }
    else{
        hit++;
        rec.erase(pages[ref]);
    }
    rec.push_front(ref);
    pages[ref] = rec.begin();
}
