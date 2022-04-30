#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <dirent.h>

#define FILE_NAME_LEN 64

int cpf2f(char* file1, char* file2)//file2 either exists or does't exist
{   
    //printf("file1 is: %s, file2 is: %s\n",file1, file2);
    struct stat mystat;
    if(lstat(file1, &mystat) < 0){//must be lstat, not stat
        printf("stat %s fail\n", file1);
        exit(-1);
    }
    printf("%s st_mode is %x\n", file1, mystat.st_mode);
    if(S_ISLNK(mystat.st_mode)){
        char linkName[FILE_NAME_LEN], fullLinkName[FILE_NAME_LEN];
        strcpy(fullLinkName,file2);
        readlink(file1, linkName, FILE_NAME_LEN);
        for(int i=strlen(fullLinkName)-1;i>=0;i--){
            if(fullLinkName[i] == '/'){
                fullLinkName[i+1] = 0;
                break;
            }
        }
        strcat(fullLinkName, linkName);
        symlink(fullLinkName, file2);
        printf("link file %s to file %s\n", fullLinkName, file2);
    }
    else if(S_ISREG(mystat.st_mode)){
        int oldFd = open(file1, O_RDONLY);
        int newFd = open(file2, O_RDWR | O_CREAT | O_TRUNC, 0664);
        int n=0;
        char buf[4096];
        while(n = read(oldFd, buf, 4096)){
            write(newFd, buf, n);
        }
        close(oldFd);
        close(newFd);
        printf("copy file %s to file %s\n", file1, file2);
    }
}

int cpf2d(char* file, char* dir)//the precondition is the existance of dir
{   
    printf("copy file %s to directory %s\n", file, dir);
    //printf("file1 is: %s, dir is: %s\n",file, dir);
    char newFile[FILE_NAME_LEN]={0};
    strcpy(newFile, dir);
    strcat(newFile, "/");
    strcat(newFile, basename(file));
    cpf2f(file, newFile);
}

int cpd2d(char* dir1, char* dir2)
{
    int d2d_dstExist = 0;
    struct stat d2d_stat;
    if(stat(dir2, &d2d_stat) < 0){
        d2d_dstExist = 0;
    }
    else{
        d2d_dstExist = 1;
    }

    char mkdirName[FILE_NAME_LEN]={0};
    if(d2d_dstExist){
        strcpy(mkdirName, dir2);
        strcat(mkdirName, "/");
        strcat(mkdirName, basename(dir1));
        if(mkdir(mkdirName, S_IRWXU | S_IRWXG | S_IRWXO) < 0){
            printf("mkdir: %s fail\n", mkdirName);
            exit(-1);
        }
        printf("copy directory %s to directory %s\n", dir1, mkdirName);
        //printf("dir1 is: %s, dir2 is: %s\n",dir1, mkdirName);
    }
    else{//not exist
        printf("copy directory %s to directory %s\n", dir1, dir2);
        //printf("dir1 is: %s, dir2 is: %s\n",dir1, dir2);
        if(mkdir(dir2, S_IRWXU | S_IRWXG | S_IRWXO) < 0){
            printf("mkdir: %s fail\n", dir2);
            exit(-1);
        }
        strcpy(mkdirName, dir2);
    }
    DIR* dp = opendir(dir1);
    if(dp == NULL){
        printf("opendir %s fail\n", dir1);
        exit(-1);
    }
    struct dirent *dtp; 
    while(dtp = readdir(dp)){
        if(strcmp(dtp->d_name, ".") == 0 || strcmp(dtp->d_name, "..") == 0){//exclude "." and ".."
            continue;
        }
        char childFile[FILE_NAME_LEN];
        strcpy(childFile, dir1);
        strcat(childFile, "/");
        strcat(childFile, dtp->d_name);
        struct stat myStat;
        if(stat(childFile, &myStat) < 0){
            printf("stat fail\n");
            exit(-1);
        }
        if(S_ISDIR(myStat.st_mode)){ 
            cpd2d(childFile, mkdirName);
        }
        else{//regular file or link file
            cpf2d(childFile, mkdirName);
        }
    }
}


int main(int argc, char *argv[])
{
    struct stat stat_src, stat_dst;
    char cwd[FILE_NAME_LEN];
    char src[FILE_NAME_LEN] , dst[FILE_NAME_LEN];
    int dstExist = 0;
    getcwd(cwd,FILE_NAME_LEN);
    if(argv[1][strlen(argv[1])-1] == '/'){
        argv[1][strlen(argv[1])-1] = 0;
    }
    if(argv[2][strlen(argv[2])-1] == '/'){
        argv[2][strlen(argv[2])-1] = 0;
    }
    if(argv[1][0] != '/'){
        strcpy(src, cwd);
        strcat(src, "/");
        strcat(src, argv[1]);
    }
    if(argv[2][0] != '/'){
        strcpy(dst, cwd);
        strcat(dst, "/");
        strcat(dst, argv[2]);
    }
    
    if(argc<3){
        printf("Usage: ./mycp.out src dst\n");
        exit(1);
    }
    if(stat(src,&stat_src) < 0){
        if(errno == ENOENT){
            printf("source file/directory does't exist\n");
            exit(2);
        }
    }
    //errno = 0;
    if(stat(dst,&stat_dst) < 0){
        if(errno == ENOENT){
            printf("dst does't exist\n");
            dstExist = 0;
        }
        else {
            printf("there is other problem in dst\n");
            exit(3);
        }    
    }
    else {
        dstExist = 1;
    }
    printf("dstExist is: %d\n", dstExist);
    if(stat_src.st_ino == stat_dst.st_ino){
        printf("source is the same as destiny\n");
        exit(4);
    }

    if(S_ISDIR(stat_src.st_mode)){
        cpd2d(src, dst);
    }
    else{
        if(dstExist){
            
            if(S_ISDIR(stat_dst.st_mode)){
                cpf2d(src, dst);
            }
            else if(S_ISREG(stat_dst.st_mode)){
                cpf2f(src, dst);
            }
            else{//link
                char linkName[FILE_NAME_LEN]={0};
                readlink(dst, linkName, FILE_NAME_LEN);
                cpf2f(src, linkName);
            }
        }
        else{//dst is certainly a file, rather a directory
            cpf2f(src,dst);
        }
    }
    printf("copy done\n");
    return 0;

}