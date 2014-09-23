# include<sys/time.h>
# include <stdio.h>
# include <string.h>
# include <time.h>
# include <unistd.h>
# include <fcntl.h>
# include <iostream>
# include "src/city.h"
int main(int argc, char*argv[])
{
	int fd = open("/dev/urandom", O_RDONLY);
	char data[4096];
	read(fd, &data, 4096);
	close(fd);
        struct timespec tv1, tv2;
        uint64_t numHashes;
        std::cin >> numHashes;
        clock_gettime(CLOCK_MONOTONIC, &tv1);
        for (int i = 0; i < numHashes; ++i) {
            CityHash128(data, 4096);
        }
        clock_gettime(CLOCK_MONOTONIC, &tv2);
        uint64_t tv = (tv2.tv_sec - tv1.tv_sec) * 1000000000+ tv2.tv_nsec -tv1.tv_nsec;
        printf("Total hash time = %ld.%06ld s\n", tv / 1000000000, tv % 1000000000);
        return 0;
} 
