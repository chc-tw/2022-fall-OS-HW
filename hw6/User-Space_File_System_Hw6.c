#define FUSE_USE_VERSION 30
#include <stdlib.h>
#include <stdio.h>
#include <fuse.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
struct header{
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char checksum[8];
        char typeflag[1];
        char linkname[100];
        char magic[6];
        char version[2];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];
        char pad[12];
} *file;
typedef struct FileNode{
    char name[100]; 
    char path[100]; 
    int flag;
    int filenumber;
    struct FileNode** files; 
    struct FileNode* pNode;
    int index;
}FileNode, *FileTree;

FileTree init(){
    FileTree root;
    root = (FileNode *)malloc(sizeof(FileNode));
    strcpy(root->name, "/");
    strcpy(root->path, "/");
    root->flag = 0;
    root->files = NULL;
    root->pNode = NULL;
    root->index = -1;
    return root;
}

FileNode * makeFile(FileNode *pNode, char dirname[],int flag,int index){
    if(pNode->files == NULL){
        pNode->files = (FileNode**)malloc(20 * sizeof(FileNode*));
    }
    pNode->files[pNode->filenumber] = (FileNode *)malloc(sizeof(FileNode));
    FileNode *dir = pNode->files[pNode->filenumber++];
    dir->flag = flag;
    strcpy(dir->name, dirname);
    dir->filenumber = 0;
    dir->files = NULL;
    dir->pNode = pNode;
    dir->index = index;
    return dir;
}

FileNode *findFile(FileNode *pNode, char *dirname,int flag){
        FileNode* curNode = NULL;
        int i;
        for(i = 0; i < pNode->filenumber; i++){
                if(strcmp(pNode->files[i]->name, dirname) == 0
                && pNode->files[i]->flag == flag){
                curNode = pNode->files[i];
                return curNode;
                }
        }
        return NULL;
}

void showDir(FileNode *pNode){
    int i;
    for(i = 0; i < pNode->filenumber; i++){
        printf("%s ", pNode->files[i]->name);
    }
    printf("\n");
}

int isDir(char *name){
        for(int i = 0; i <strlen(name); i++){
                if(name[i] == '.'||name[i] == '_'){
                        return 0;
                }
        }
        return 1;
}

FileTree root;

off_t foffset[1000];
struct header *flist[1000];
char fname[100][100];
char *substr[100] = {NULL};
char *saveptr[100] = {NULL};
int out[100]={0};
int nfiles = 0,tarsz;
int fd;
void Partition( struct header *file){
        root = init();
        int length;
        while(file->name[0]){
                //strncpy(flist[nfiles] , file,512);
                flist[nfiles] = file;
                sscanf( file->size, "%o", &length);
                strcpy(fname[nfiles],flist[nfiles]->name);
                printf("te:%s\n",fname[nfiles]);
                file += 1 + (length+511)/512 ;
                foffset[nfiles] = lseek(fd, 0, SEEK_CUR);
                printf("%ld\n",foffset[nfiles]);
                lseek(fd, 512 + (length + 512 - 1) / 512 * 512, SEEK_CUR);
                nfiles ++;
        }
        
        //build the file tree
        for(int i = 0; i < nfiles; i++){
                FileNode* curNode = root;
                substr[i] = strtok_r(fname[i],"/",&saveptr[i]);
                while(substr[i]!=NULL){
                        char temp[100];
                        strcpy(temp,substr[i]);
                        temp[strlen(temp)] = '\0';
                        int flag = isDir(substr[i]);
                        //printf("%s %d\n",substr[i],flag);
                        FileNode* Node = findFile(curNode,substr[i],flag);
                        substr[i] = strtok_r(NULL,"/",&saveptr[i]);
                        if(Node == NULL && substr[i]==NULL){
                                //printf("creat:%s\n",temp);
                                Node = makeFile(curNode,temp,flag,i);
                        } 
                        if(Node == NULL && substr[i]!=NULL){
                                //printf("creat:%s\n",temp);
                                Node = makeFile(curNode,temp,flag,-1);
                        } 
                        if(substr[i]==NULL && Node->index==-1){
                                Node->index = i;
                        }
                        if(substr[i]==NULL && Node!=NULL){
                               Node->index = i;
                        }
                        curNode = Node;
                }
        }
}
void mapTar(const char* path){
  FILE* fptr = fopen(path,"r");
  fseek(fptr, 0L, SEEK_END);
  tarsz = ftell(fptr);
  fclose(fptr);
  fd=open(path, O_RDONLY);
  file=mmap(NULL, tarsz, PROT_READ, MAP_PRIVATE, fd, 0);
}

int my_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
        FileNode *node=root,*curnode = root;
        if ( strcmp( path, "/" ) != 0 ){
                char* subpath,*savepath;
                subpath = strtok_r(path,"/",&savepath);
                while(subpath != NULL){
                        node = findFile(curnode,subpath,1);
                        if(node == NULL) return -ENOENT;
                        curnode = node;
                        subpath = strtok_r(NULL,"/",&savepath);
                }
        }
        
        filler(buffer, ".", NULL, 0);
        filler(buffer, "..", NULL, 0);
        for(int i = 0; i <node->filenumber;i++){
                filler(buffer,node->files[i]->name,NULL,0);
        }
        return 0;  
}
int my_getattr(const char *path, struct stat *st) {
        //for(int i = 0; i<nfiles; i++) printf("%s ",flist[i]->name);
        memset(st, 0, sizeof(struct stat));
        if(strcmp(path,"/") == 0){
                st->st_uid = getuid();
                st->st_gid = getgid();
                st->st_atime = time( NULL );
                st->st_mtime = time( NULL );
                st->st_mode = S_IFDIR | 0444;
                //st->st_nlink = 2;
                return 0;
        }
        else{   
                path++;
                FileNode* node,*curnode = root;
                char* subpath,*savepath;
                subpath = strtok_r(path,"/",&savepath);
                while(subpath != NULL){
                        node = findFile(curnode,subpath,1);
                        if(node == NULL){
                                node = findFile(curnode,subpath,0);
                        }
                        if(node == NULL) return -ENOENT;
                        curnode = node;
                        subpath = strtok_r(NULL,"/",&savepath);
                }
                int i =node->index;
                printf("%s:i=%d\n",flist[i]->name,i);
                st->st_uid=strtol(flist[i]->uid,NULL,8);
                st->st_gid=strtol(flist[i]->gid,NULL,8);
                st->st_mtime=strtol(flist[i]->mtime,NULL,8);
                st->st_size=strtol(flist[i]->size,NULL,8);        
                //st->st_mode=strtol(flist[i]->mode,NULL,8);
                int len = (strlen(flist[i]->name)-1);
                int mode=strtol(flist[i]->mode,NULL,8);
                //st->st_mode = S_IFREG | mode;
                //printf("attr:%s\n",flist[i]->name+len);
                
                if(node->flag){
                        st->st_mode = S_IFDIR | mode;
                        // return -ENOENT;
                }
                else{
                         st->st_mode = S_IFREG | mode;
                }
                return 0;
        }
 }
int my_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
        path++;
        FileNode* node,*curnode = root;
        char* subpath,*savepath;
        subpath = strtok_r(path,"/",&savepath);
        while(subpath != NULL){
                node = findFile(curnode,subpath,1);
                if(node == NULL){
                        node = findFile(curnode,subpath,0);
                }
                if(node == NULL) return -ENOENT;
                curnode = node;
                subpath = strtok_r(NULL,"/",&savepath);
        }
        int i = node->index;
        int length = strtol(flist[i]->size,NULL,8);
        if (offset >= length) {
                // Offset is past the end of the file
                return 0;
            }
        lseek(fd, foffset[i] + 512 + offset, SEEK_SET);

        // Read data from tar file
        return read(fd, buffer, size);

}
static struct fuse_operations op;
int main(int argc, char *argv[]){
        mapTar("test.tar");
        Partition(file);
        //parse_tar_file("hw6/test/test.tar");
        memset(&op, 0, sizeof(op));
        op.getattr = my_getattr;
        op.readdir = my_readdir;
        op.read = my_read;
        return fuse_main(argc, argv, &op, NULL);
}
