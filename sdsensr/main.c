#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <inttypes.h>
#include <semaphore.h>

// Uncomment for nonblocking bus mode
//#define NONBLOCK

#ifndef NONBLOCK
sem_t *smph;
char *SEM_NAME;
#endif

enum {
	DATA_INT,
	DATA_FLOAT,
};

int bus = -1;
int addr = -1;
int (*getdata)(int *data);
int (*getdataf)(long double *data);
int datatype = DATA_INT;

int opendev(int flags);
int parseopts(int argc, char *argv[]);
int usage(char *name);

int get8(int *data);
int get16(int *data);
int get32(int *data);
int getu8(int *data);
int getu16(int *data);
int getu32(int *data);
int getf32(long double *data);
int getf64(long double *data);
int getf80(long double *data);
int getbytes(char *buf, int size);

int main(int argc, char *argv[])
{
	getdata = get8;
	getdataf = getf32;
	parseopts(argc, argv);
	if (bus < 0 || addr < 0) {
		usage(argv[0]);
		exit(1);
	}

#ifndef NONBLOCK
	SEM_NAME = (char*)malloc(15);
	sprintf(SEM_NAME, "i2c_bus_%d_sem", bus);
	smph = sem_open(SEM_NAME,O_CREAT,0644,1);
	if (smph == SEM_FAILED) {
		fputs("unable to create semaphore\n", stderr);
		sem_unlink(SEM_NAME);
		free(SEM_NAME);
		exit(-1);
	}

	sem_wait(smph);
#endif

	int d = 0;
	long double df = 0;
	int n = 0;

	switch (datatype) {
		case DATA_FLOAT:
			n = getdataf(&df);
			break;

		case DATA_INT:
		default :
			n = getdata(&d);
			break;
	}

	if (n <= 0) {
#ifndef NONBLOCK
		sem_post(smph);
		sem_close(smph);
		free(SEM_NAME);
#endif
		exit(1);
	}

	switch (datatype) {
		case DATA_FLOAT:
			printf("%f\n", df);
			break;

		case DATA_INT:
		default :
			printf("%d\n", d);
			break;
	}

#ifndef NONBLOCK
	sem_post(smph);
	sem_close(smph);
	free(SEM_NAME);
#endif

	return 0;
}

int parseopts(int argc, char *argv[])
{
	char *optstring = "b:a:t:";
	struct option longopts[] = {
		{ "bus",         required_argument, NULL, 'b' },
		{ "address",     required_argument, NULL, 'a' },
		{ "type",        required_argument, NULL, 't'},
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
			case 't':
				switch (*optarg) {
				case 'i':
					datatype = DATA_INT;
					if (strcmp(optarg + 1, "8") == 0) {
						getdata = get8;
					} else if (strcmp(optarg + 1, "16") == 0) {
						getdata = get16;
					} else if (strcmp(optarg + 1, "32") == 0) {
						getdata = get32;
					};
					break;
				case 'u':
					datatype = DATA_INT;
					if (strcmp(optarg + 1, "8") == 0) {
						getdata = getu8;
					} else if (strcmp(optarg + 1, "16") == 0) {
						getdata = getu16;
					} else if (strcmp(optarg + 1, "32") == 0) {
						getdata = getu32;
					};
					break;
				case 'f':
					datatype = DATA_FLOAT;
					if (strcmp(optarg + 1, "32") == 0) {
						getdataf = getf32;
					} else if (strcmp(optarg + 1, "64") == 0) {
						getdataf = getf64;
					} else if (strcmp(optarg + 1, "80") == 0) {
						getdataf = getf80;
					};
					break;
				default :
					usage(argv[0]);
					free(endp);
					exit(1);
					break;
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
	return fprintf(stderr, "usage: %s --bus=<bus> --address=<addr> [--type=<type>]\n", name);
}

int get8(int *data)
{
	int8_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		free(buf);
		return 0;
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);

	*data = (int)d;
	return size;
}

int get16(int *data)
{
	int16_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		free(buf);
		return 0;
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);

	*data = (int)d;
	return size;
}

int get32(int *data)
{
	int32_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		free(buf);
		return 0;
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);

	*data = (int)d;
	return size;
}

int getu8(int *data)
{
	uint8_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		free(buf);
		return 0;
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);

	*data = (int)d;
	return size;
}

int getu16(int *data)
{
	uint16_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		free(buf);
		return 0;
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);

	*data = (int)d;
	return size;
}

int getu32(int *data)
{
	uint32_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		free(buf);
		return 0;
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);

	*data = (int)d;
	return size;
}

int getf32(long double *data)
{
	float d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		free(buf);
		return 0;
	}

	*data = *((float *)buf);

	free(buf);

	return size;
}

int getf64(long double *data)
{
	double d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		free(buf);
		return 0;
	}

	*data = *((double *)buf);

	free(buf);

	return size;
}

int getf80(long double *data)
{
	long double d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		free(buf);
		return 0;
	}

	*data = *((long double *)buf);

	free(buf);

	return size;
}

int getbytes(char *buf, int size)
{
	int f;
	if ((f = opendev(O_RDONLY)) < 0) {
		return 0;
	}
	int n = read(f, buf, size);
	close(f);
	return n;
}

int opendev(int flags)
{
	int f;
	char *filepath;
	filepath = (char*)malloc(12);
	sprintf(filepath, "/dev/i2c-%d", bus);
	if ((f = open(filepath, flags)) < 0) {
		fprintf(stderr, "Can not open file `%s'\n", filepath);
		free(filepath);
		return f;
	}
	free(filepath);
	if (ioctl(f, I2C_SLAVE, addr) < 0) {
		fputs("Failed to acquire bus access and/or talk to slave.\n", stderr);
		close(f);
		return -1;
	}
	return f;
}
