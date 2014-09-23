# include <sys/time.h>
# include <time.h>
# include <signal.h>
# include <errno.h>
# include <sys/wait.h>
# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <libcgroup.h>
# include <iostream>
# include "src/city.h"
# include <deque>
# include <string.h>
# include <string>

int  computeHash (void * arg); 

extern "C" 
{
int start(int numProcesses);
int stop();
}
struct HashArgs
{
 char s[4096];
 size_t len;
 uint64_t numHashes;
 HashArgs(const char* m_s, size_t m_len, uint64_t m_numHashes)
        : len(m_len), numHashes(m_numHashes)
        {
          memcpy((void*) s, (void *)m_s, len);
        }

};

int computeHash (void * arg) 
{
	pause();
        //char tmp[6] = "city\n";
	//std::cout << "inside cityhash" << std::endl;
	//write(STDOUT_FILENO, tmp, 5);
	HashArgs* hashArgs = (HashArgs *) arg; 
        for (uint64_t  i = 0; i < hashArgs->numHashes; ++i) {
            CityHash128(hashArgs->s, hashArgs->len);
        }
       return 0;
}

void sigHandler(int sigNo)
{
    return; 
}

int main(int argc, char*argv[])
{
	std::string configFile = "/etc/cgconfig.conf";
	cgroup_init();
	cgroup_config_load_config(configFile.c_str());
	struct cgroup* hipri = cgroup_new_cgroup("hipri");
	struct cgroup* lopri = cgroup_new_cgroup("lopri");
	cgroup_get_cgroup(hipri);
	cgroup_get_cgroup(lopri);
	cgroup_attach_task(lopri);
	signal(SIGCONT, sigHandler);
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
	std::deque<HashArgs> argsList;
        struct timespec tv1, tv2;
        uint64_t numHashes;
        uint64_t numThreads;
        uint64_t numBackground;
	const size_t stackSize = 16384;
        std::cout << "numHashes numThreads numBg" << std::endl;
        std::cin >> numHashes;
        std::cin >> numThreads;
        std::cin >> numBackground;
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
	    argsList.emplace_back(data, 4096, numHashes);
            pid = clone(computeHash, (char *)stacks[i] + stackSize, CLONE_VM | SIGCHLD, (void*)&argsList[i]);
	    if (pid == -1) {
		perror("clone error");
		exit(3);
	    }
	    std::cout << "created process " << pid << std::endl;
	    pids.push_back(pid);
        }
	sleep(1); // wait for all LWPs to set up before sending SIG
        clock_gettime(CLOCK_MONOTONIC, &tv1);
	for (uint64_t i = 0; i < numThreads; ++i) {
	    cgroup_attach_task_pid(hipri, pids[i]);
	    if (kill(pids[i], SIGCONT) == -1) {
		printf("failed to start process %d\n", pids[i]);
	    }
	}
        for (uint64_t i = 0; i < numThreads; ++i) {
          // std::cout << "waiting for process " << pids[i] << std::endl;
	  // char tmp[6] = "wait\n";
	  // write(STDOUT_FILENO, tmp, 5);
	   int pid;
	   pid = waitpid(pids[i], 0, 0);
	   if (pid == -1) {
		perror("waitpid");
		exit(4);
	   }
  	}
        clock_gettime(CLOCK_MONOTONIC, &tv2);
        //std::cout << "stopping background ..." << std::endl;
        stop();
	for (uint64_t i = 0; i < numThreads; ++i) {
	    free(stacks[i]);
	}
        uint64_t tv = (tv2.tv_sec - tv1.tv_sec) * 1000000000 + tv2.tv_nsec -tv1.tv_nsec;
        printf("Total hash time = %ld.%06ld s\n", tv / 1000000000, tv % 1000000000);
        return 0;
} 
