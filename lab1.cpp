#include <getopt.h>
# include <sys/time.h>
# include <time.h>
# include <string.h>
# include <signal.h>
# include <errno.h>
# include <sys/wait.h>
# include <stdio.h>
# include <unistd.h>
# include <sched.h>
# include <fcntl.h>
# include <iostream>
# include "src/city.h"
# include <deque>
using namespace std; 

static void usageAndExit(const char* const executable, const int exitValue)
{
    ((exitValue == EXIT_SUCCESS) ? cout : cerr)
        << "Usage: " << executable << " [options]\n"
        << " -t --hash-threads        number of hashing processes\n"
        << " -b --background          number of background processes\n"
        << " -h --help                print this help message\n"
        << " -n --num-hashes          number of hashes\n"
        << endl;
    exit(exitValue);
}

int  computeHash (void * arg); 

extern "C" 
{
int start(int numProcesses);
int stop();
}
struct HashArgs
{
 char  s[4096];
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
	//cpu_set_t set;
	//sched_getaffinity(0, sizeof(cpu_set_t), &set);
        // char tmp[6] = "city\n";
       	//for (int i =0; i < CPU_SETSIZE; ++i)
	//std::cout << "cpu 0 :" << CPU_ISSET(0, &set) << std::endl;
	//std::cout << "cpu 1 :" << CPU_ISSET(1, &set) << std::endl;
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
    uint64_t numHashes;
    uint64_t numThreads;
    uint64_t numBackground;
    struct option longOptions[] = {
        {"hash-threads", required_argument, 0, 't'},
        {"background", required_argument, 0, 'b'},
        {"num-hashes", required_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    const char* executable = basename(argv[0]);
    int c;
    while ((c = getopt_long(argc, argv, "b:t:n:h:", longOptions, nullptr)) != -1)
    {
        switch(c) {
        case 'b':
            numBackground = strtoul(optarg, NULL, 0);
            break;
        case 't':
            numThreads = strtoul(optarg, NULL, 0);
            break;
        case 'h':
            usageAndExit(executable, EXIT_SUCCESS);
            break;
        case 'n':
            numHashes = strtoul(optarg, NULL, 0);
            break;
        default:
            usageAndExit(executable, EXIT_FAILURE);
	    break;
        }
    }

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
	const size_t stackSize = 16384;
	cpu_set_t set;
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
  	    int pid, ret; 
	    argsList.emplace_back(data, 4096, numHashes);
            pid = clone(computeHash, (char *)stacks[i] + stackSize, CLONE_VM | SIGCHLD, (void*)&argsList[i]);
	    if (pid == -1) {
		perror("clone error");
		exit(3);
	    }
	    std::cout << "created process " << pid << std::endl;
	    pids.push_back(pid);
	    CPU_ZERO(&set);
	    CPU_SET(i, &set);
	    ret = sched_setaffinity(pid, sizeof(cpu_set_t), &set);
	    if (ret == -1) 
		perror("sched_setaffinity");
        }
	sleep(1); // wait for all LWPs to set up before sending SIG
        //gettimeofday(&tv1, NULL);
	clock_gettime(CLOCK_MONOTONIC, &tv1);
	for (uint64_t i = 0; i < numThreads; ++i) {
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
        //gettimeofday(&tv2, NULL);
        //std::cout << "stopping background ..." << std::endl;
	clock_gettime(CLOCK_MONOTONIC, &tv2);
        stop();
	for (uint64_t i = 0; i < numThreads; ++i) {
	    free(stacks[i]);
	}
        uint64_t tv = (tv2.tv_sec - tv1.tv_sec) * 1000000000 + tv2.tv_nsec -tv1.tv_nsec;
        printf("Total hash time = %ld.%06ld s\n", tv / 1000000000, tv % 1000000000);
        return 0;
} 
