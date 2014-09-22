# include<sys/time.h>
# include <errno.h>
# include<sys/wait.h>
# include <stdio.h>
# include<unistd.h>
# include<fcntl.h>
# include<iostream>
# include "src/city.h"
# include<deque>
int  computeHash (void * arg); 

extern "C" 
{
int start(int numProcesses);
int stop();
}
struct HashArgs
{
 const char * s;
 size_t len;
 uint64_t numHashes;
};

int computeHash (void * arg) 
{
	std::cout << "inside cityhash" << std::endl;
	HashArgs* hashArgs = (HashArgs *) arg; 
        for (uint64_t  i = 0; i < hashArgs->numHashes; ++i) {
            CityHash128(hashArgs->s, hashArgs->len);
        }
       return 0;
}

int main(int argc, char*argv[])
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
	    perror("error reading /dev/urandom");
	    exit(1);
  	}
	char data[4096];
	read(fd, &data, 4096);
	close(fd);
	std::deque<int> pids;
	std::deque<void *> stacks;
        struct timeval tv1, tv2;
        uint64_t numHashes;
        uint64_t numThreads;
        uint64_t numBackground;
	const size_t stackSize = 16384;
        std::cout << "numHashes numThreads numBg" << std::endl;
        std::cin >> numHashes;
        std::cin >> numThreads;
        std::cin >> numBackground;
        HashArgs hashArgs;
        hashArgs.s = data;
        hashArgs.len = 4096;
        hashArgs.numHashes = numHashes;
        void* arg = (void*)&hashArgs;
        start (numBackground);
	for (uint64_t i = 0; i < numThreads; ++i) {
	    void * child_stack = malloc(stackSize);
	    if (child_stack == 0) {
		perror("malloc: could not allocate stack");
		exit(2);
	    } 
	    stacks.push_back(child_stack);
	}
        for (uint64_t i = 0; i < numThreads; ++i) {
  	    int pid; 
            pid = clone(computeHash, (char *)stacks[i] + stackSize, CLONE_VM | SIGCHLD, arg);
            if (pid == -1) {
		perror("clone error");
		exit(3);
	    }
	    std::cout << "created process " << pid << std::endl;
	    pids.push_back(pid);
        }
        for (uint64_t i = 0; i < numThreads; ++i) {
           std::cout << "waiting for process " << pids[i] << std::endl;
	   int pid;
	   pid = waitpid(pids[i], 0, 0);
	   if (pid == -1) {
		perror("waitpid");
		exit(4);
	   }
  	}
        std::cout << "stopping background ..." << std::endl;
        stop();
	for (uint64_t i = 0; i < numThreads; ++i) {
	    free(stacks[i]);
	}
        gettimeofday(&tv1, NULL);
        gettimeofday(&tv2, NULL);
        uint64_t tv = (tv2.tv_sec - tv1.tv_sec) * 1000 * 1000 + tv2.tv_usec -tv1.tv_usec;
        printf("Total hash time = %ld.%06ld s\n", tv / 1000000, tv % 1000000);
        return 0;
} 
