#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

pthread_mutex_t the_mutex = PTHREAD_MUTEX_INITIALIZER;

size_t bytes_read = 0;
size_t new_size = 0;
char* pOut;
struct stat statinfo;

void* read2(void *path){    
    size_t tries = 0;    
    off_t len = 0;
    int fIn = 0;
    
    fIn = open(path, O_RDWR | O_CREAT | O_TRUNC);
    if (fIn == -1) {
        perror ("open");
        exit(EXIT_FAILURE);
    }
    
    for(len = 0; len < 200; len++) {
        pthread_mutex_lock(&the_mutex);
        if(pOut[len] == 0){
            if(tries++ == 5){
                pthread_mutex_unlock(&the_mutex);
                break;
            }
            else {
                pthread_mutex_unlock(&the_mutex);
                continue;
            }
        }  
        write(fIn, &pOut[len], 1);
        pOut[len] = 0;
        pthread_mutex_unlock(&the_mutex);
        usleep(1);
    }
    close(fIn);
}

void* write2(void *path){
    char buff = 0;    
    int fIn = 0;
    
    fIn = open(path, O_RDONLY);
    if (fIn == -1) {
        perror ("open");
        exit(EXIT_FAILURE);
    }
    
    while(read(fIn, &buff, 1)){ 
        pthread_mutex_lock(&the_mutex);
        pOut[new_size] = buff;
        new_size += 1;
        buff = 0;
        if(bytes_read++ > 200){
            pthread_mutex_unlock(&the_mutex);
            break;
        }
        pthread_mutex_unlock(&the_mutex);
        usleep(1);
    }

    close(fIn);
}


int main(){
    pthread_t tr1, tr2, tr3, tr4;
    int ifOut = 0;
    int ret = 0;

    if((ifOut = open("Out.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH)) < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    if(ftruncate(ifOut, sizeof(char) * 200) != 0){
            perror("ftruncate");
            exit(EXIT_FAILURE);
    }

    if((ret = fstat(ifOut, &statinfo)) < 0){
        perror("fstat");
        return EXIT_FAILURE;
    }

    if ((pOut = (char*)mmap(0, statinfo.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, ifOut, 0)) == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}
    
    pthread_create(&tr1, NULL, write2, "One.txt");
    pthread_create(&tr2, NULL, write2, "Two.txt");
    pthread_create(&tr3, NULL, read2, "Three.txt");
    pthread_create(&tr4, NULL, read2, "Four.txt");

    pthread_join(tr1, NULL);
    pthread_join(tr2, NULL);
    pthread_join(tr3, NULL);
    pthread_join(tr4, NULL);

    if((msync(pOut, 200, MS_SYNC)) < 0)
        perror("msync");
    if(munmap(pOut, 200) == -1)
        perror("munmap"); 

    close(ifOut);

    return 0;
}

