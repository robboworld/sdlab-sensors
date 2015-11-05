/*
 Sample code for the BH1750 Light sensor for Raspberry Pi
 Connection:
 VCC-5v or 3.3v (Raspberry pin 1)
 GND-GND(Raspberry pin 6)
 SCL-SCL(Raspberry pin 5)
 SDA-SDA(Raspberry pin 3)
 ADD-NC or GND
 */
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <getopt.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <semaphore.h>

#define BH1750FVI_I2C_ADDRESS 0x23 // ADDR > 0.7 VCC)
//#define BH1750FVI_I2C_ADDRESS  0x53 // ADDR < 0.3 VCC) 
#define DEBUG 0
//#define DATAWAIT 2000000  // Wait for data in usec (2sec)
#define DATAWAIT 200000  // Wait for data in usec (200ms)

#define PowerDown    0x00
#define PowerOn    0x01
#define Reset      0x07
#define ContinuHigh   0x10
#define ContinuLow   0x13
#define OneTimeHigh   0x20
#define OneTimeLow   0x23

// Uncomment for nonblocking bus mode
//#define NONBLOCK

int bus = -1;
int addr = -1;

int parseopts(int argc, char *argv[]);

int main(int argc, char **argv)
{
	int fd;
	int retCode;
	int readSize;
	unsigned int res;
	unsigned int lux;
	char buf[5];
	char wbuf[1];
	char *filepath;
	int i;

#ifndef NONBLOCK
	sem_t *smph;
	char *SEM_NAME;
#endif

	parseopts(argc, argv);
	if (addr < 0) {
		addr = BH1750FVI_I2C_ADDRESS;
	}

	if (bus < 0 || addr < 0) {
		usage(argv[0]);
		exit(1);
	}

#ifndef NONBLOCK
	// init semaphore
	SEM_NAME = (char*)malloc(15);
	sprintf(SEM_NAME, "i2c_bus_%d_sem", bus);
	smph = sem_open(SEM_NAME,O_CREAT,0644, 1);
	if (smph == SEM_FAILED) {
		fputs("unable to create semaphore.\n", stderr);
		sem_unlink(SEM_NAME);
		free(SEM_NAME);
		exit(-1);
	}

	sem_wait(smph);
#endif

	filepath = (char*)malloc(12);
	sprintf(filepath, "/dev/i2c-%d", bus);

	// Open port for reading and writing
	if ((fd = open(filepath, O_RDWR)) < 0) {
		fprintf(stderr, "Can not open file '%s'\n", filepath);
		free(filepath);
#ifndef NONBLOCK
		sem_post(smph);
		sem_close(smph);
		free(SEM_NAME);
#endif
		exit(1);
	}
	free(filepath);

	// Set the port options and set the address of the device 
	if (ioctl(fd, I2C_SLAVE, addr) < 0) {
		fputs("Failed to acquire bus access and/or talk to slave.\n", stderr);
		close(fd);
#ifndef NONBLOCK
		sem_post(smph);
		sem_close(smph);
		free(SEM_NAME);
#endif
		exit(1);
	}

	wbuf[0] = PowerOn;
	retCode = write(fd, (void *)wbuf, 1);
if(DEBUG)printf("Power On retCode=%d\n",retCode);
	if (retCode < 0) {
		fprintf(stderr, "PowerOn error (%d)\n", retCode);
		//printf("PowerOn error\n");
		close(fd);
#ifndef NONBLOCK
		sem_post(smph);
		sem_close(smph);
		free(SEM_NAME);
#endif
		exit(1);
	}

/*
	wbuf[0] = ContinuHigh;
	retCode = write(fd, (void *)wbuf, 1);
if(DEBUG)printf("ContinuHigh retCode=%d\n",retCode);
	if (retCode < 0) {
		fprintf(stderr, "ContinueHigh error (%d)\n", retCode);
		//printf("ContinuHigh error\n");
		close(fd);
#ifndef NONBLOCK
		sem_post(smph);
		sem_close(smph);
		free(SEM_NAME);
#endif
		exit(1);
	}
*/

	wbuf[0] = OneTimeHigh;
	retCode = write(fd, (void *)wbuf, 1);
if(DEBUG)printf("OneTimeHigh retCode=%d\n",retCode);
	if (retCode < 0) {
		fprintf(stderr, "OneTimeHigh error (%d)\n", retCode);
		//printf("OneTimeHigh error\n");
		close(fd);
#ifndef NONBLOCK
		sem_post(smph);
		sem_close(smph);
		free(SEM_NAME);
#endif
		exit(1);
	}

	// Wait for data
	usleep(DATAWAIT);

	readSize = read(fd, buf, 2);
if(DEBUG)printf("read readSize=%d %#x %#x\n",readSize,buf[0],buf[1]);

	// Calculate value in lux
	res = buf[0]*256+buf[1];
if(DEBUG)printf("res=%#x\n",res);
	lux = res / 1.2;

	// Output result
	printf("%u\n",lux);

	wbuf[0] = PowerDown;
	retCode = write(fd, (void *)wbuf, 1);
	close(fd);

#ifndef NONBLOCK
	sem_post(smph);
	sem_close(smph);
	free(SEM_NAME);
#endif

	exit(0);
}

int parseopts(int argc, char *argv[])
{
	char *optstring = "b:a:th";
	struct option longopts[] = {
		{ "bus",         required_argument, NULL, 'b' },
		{ "address",     required_argument, NULL, 'a' },
		{ NULL,          0,                 NULL, 0 }
	};
	int li;
	int o;
	char** endp = malloc(sizeof(char*));
	while ((o = getopt_long(argc, argv, optstring, longopts, &li)) >= 0) {
		if (o != 0) {
			switch (o) {
			case 'b':
				bus = (int)strtol(optarg, endp, 10);
				if (**endp != '\0' || *endp == optarg) {
					usage(argv[0]);
					free(endp);
					exit(1);
				}
				break;
			case 'a':
				addr = (int)strtol(optarg, endp, 10);
				if (**endp != '\0' || *endp == optarg) {
					usage(argv[0]);
					free(endp);
					exit(1);
				}
				break;
			case '?':
			default :
				usage(argv[0]);
				free(endp);
				exit(1);
				break;
			}
		}
	}
	free(endp);
	return optind;
}

int usage(char *name)
{
	return fprintf(stderr, "usage: %s --bus=<bus> | -b <bus> --address=<addr> | -a <addr>\n", name);
}
