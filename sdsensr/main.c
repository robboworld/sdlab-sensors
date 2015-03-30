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

int bus = -1;
int addr = -1;
int (*getdata)();

int opendev(int flags);
int parseopts(int argc, char *argv[]);
int usage(char *name);

int get8();
int get16();
int get32();
int getu8();
int getu16();
int getu32();
int getbytes(char *buf, int size);

int main(int argc, char *argv[])
{
	getdata = get8;
	parseopts(argc, argv);
	if (bus < 0 || addr < 0) {
		usage(argv[0]);
		exit(1);
	}
	int d = getdata();
	printf("%d\n", d);
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
					exit(1);
				}
				break;
			case 'a':
				addr = (int)strtol(optarg, endp, 10);
				if (**endp != '\0' || *endp == optarg) {
					usage(argv[0]);
					exit(1);
				}
				break;
			case 't':
				switch (*optarg) {
				case 'i':
					if (strcmp(optarg + 1, "8") == 0) {
						getdata = get8;
					} else if (strcmp(optarg + 1, "16") == 0) {
						getdata = get16;
					} else if (strcmp(optarg + 1, "32") == 0) {
						getdata = get32;
					};
					break;
				case 'u':
					if (strcmp(optarg + 1, "8") == 0) {
						getdata =getu8;
					} else if (strcmp(optarg + 1, "16") == 0) {
						getdata =getu16;
					} else if (strcmp(optarg + 1, "32") == 0) {
						getdata =getu32;
					};
					break;
				default :
					usage(argv[0]);
					exit(1);
					break;
				}
				break;
			case '?':
			default :
				usage(argv[0]);
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

int get8()
{
	int8_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		exit(1);
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);
	return (int)d;
}

int get16()
{
	int16_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		exit(1);
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);
	return (int)d;
}

int get32()
{
	int32_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		exit(1);
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);
	return (int)d;
}

int getu8()
{
	uint8_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		exit(1);
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);
	return (int)d;
}

int getu16()
{
	uint16_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		exit(1);
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);
	return (int)d;
}

int getu32()
{
	uint32_t d = 0;
	const int size = sizeof(d);
	char *buf = (char*)malloc(size);
	char *byte;
	if (getbytes(buf, size) < size) {
		fputs("error reading data\n", stderr);
		exit(1);
	}
	for (byte = buf; byte < (buf + size); byte++) {
		d = d << 8 | (int16_t)(*byte);
	}
	free(buf);
	return (int)d;
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