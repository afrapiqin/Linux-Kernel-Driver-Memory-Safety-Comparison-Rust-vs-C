#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define DEVICE_FILE "/dev/stack_module_rust"
#define INSERT_VALUE _IOR('|', 1, int)
#define POP_VALUE _IOW('|', 2, int)

int calc_insert(int fd)
{
	int ret;
	struct timeval start, end;
	long seconds, microseconds;
	double elapsed;
	FILE *csv_file = fopen("insert_rust.csv", "w");
	if (csv_file == NULL) {
		perror("Error opening file");
		return 1;
	}
	fprintf(csv_file,
		"Data Written,Bytes Written,Elapsed Time (microseconds)\n");
	// Insert data to the device file
	char data = 1;
	for (int i = 1; i <= 256; i++) {
		gettimeofday(&start, NULL); // Record start time
		ret = ioctl(fd, INSERT_VALUE, &data);
		gettimeofday(&end, NULL); // Record end time
		if (ret < 0) {
			perror("ioctl: Failed to call into Set");
			return errno;
		}
		elapsed = end.tv_usec - start.tv_usec;
		fprintf(csv_file, "%d,%zd,%f\n", i, sizeof(data), elapsed);
		usleep(200);
	}

	return 0;
}

int calc_pop(int fd)
{
	int ret;
	struct timeval start, end;
	long seconds, microseconds;
	double elapsed;
	FILE *csv_file = fopen("pop_rust.csv", "w");
	if (csv_file == NULL) {
		perror("Error opening file");
		return 1;
	}
	fprintf(csv_file,
		"Data Written,Bytes Written,Elapsed Time (microseconds)\n");
	// Pop data from the device file
	char data;
	for (int i = 1; i <= 256; i++) {
		gettimeofday(&start, NULL); // Record start time
		ret = ioctl(fd, POP_VALUE, &data);
		gettimeofday(&end, NULL); // Record end time
		if (ret < 0) {
			perror("ioctl: Failed to call into Get");
			return errno;
		}
		elapsed = end.tv_usec - start.tv_usec;
		fprintf(csv_file, "%d,%zd,%f\n", i, sizeof(data), elapsed);
		usleep(200);
	}

	return 0;
}

int main()
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset); // Set the process to run only on CPU core 0

	if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == -1) {
		perror("sched_setaffinity");
		exit(EXIT_FAILURE);
	}

	// Open the device file for writing
	int fd, ret;
	fd = open(DEVICE_FILE, O_RDWR);
	if (fd < 0) {
		perror("Failed to open the device file");
		return errno;
	}
	printf("Device file opened successfully.\n");

	// Measure insert operation
	ret = calc_insert(fd);
	if (ret != 0) {
		perror("Insert operation failed:");
		return errno;
	}

	// Measure pop operation
	ret = calc_pop(fd);
	if (ret != 0) {
		perror("Pop operation failed:");
		return errno;
	}

	// Close the device file
	if (close(fd) < 0) {
		perror("Failed to close the device file");
		return errno;
	}

	printf("Device file closed successfully.\n");
	return 0;
}
