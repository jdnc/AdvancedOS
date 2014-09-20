# include<sys/time.h>
# include <stdio.h>
# include<unistd.h>
# include<fcntl.h>
# include<iostream>
# include "src/city.h"
int main(int argc, char*argv[])
{
	int fd = open("/dev/urandom", O_RDONLY);
	char data[4096];
	read(fd, &data, 4096);
	close(fd);
        struct timeval tv1, tv2;
        uint64_t numHashes;
        std::cin >> numHashes;
        gettimeofday(&tv1, NULL);
        for (int i = 0; i < numHashes; ++i) {
            CityHash128(data, 4096);
        }
        gettimeofday(&tv2, NULL);
        uint64_t tv = (tv2.tv_sec - tv1.tv_sec) * 1000 * 1000 + tv2.tv_usec -tv1.tv_usec;
        printf("Total hash time = %ld.%06ld s\n", tv / 1000000, tv % 1000000);
        return 0;
} 
