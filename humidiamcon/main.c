#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <semaphore.h>

// Uncomment for nonblocking bus mode
//#define NONBLOCK

enum {
	BOTH,
	TEMP,
	HUM,
};

int bus = -1;
int addr = -1;
int value = BOTH;

struct reading {
	double hum;
	double temp;
};

struct reading getdata();
int convdata(char *buf, struct reading *d);
int opendev(int flags);
int parseopts(int argc, char *argv[]);
int usage(char *name);

int main(int argc, char *argv[])
{
	parseopts(argc, argv);
	if (bus < 0 || addr < 0) {
		usage(argv[0]);
		exit(1);
	}

#ifndef NONBLOCK
	sem_t *smph;
	char *SEM_NAME = (char*)malloc(15);
	sprintf(SEM_NAME, "i2c_bus_%d_sem", bus);
	smph = sem_open(SEM_NAME,O_CREAT,0644, 1);
	if (smph == SEM_FAILED) {
		fputs("unable to create semaphore\n", stderr);
		sem_unlink(SEM_NAME);
		free(SEM_NAME);
		exit(-1);
	}

	sem_wait(smph);
#endif

	struct reading d = getdata();
	switch (value) {
	case TEMP:
		printf("%f\n", d.temp);
		break;
	case HUM:
		printf("%f\n", d.hum);
		break;
	case BOTH:
		printf("t=%f\nh=%f\n", d.temp, d.hum);
		break;
	default:
		fputs("unreachable\n", stderr);
#ifndef NONBLOCK
		sem_post(smph);
		sem_close(smph);
		free(SEM_NAME);
#endif
		exit(-1);
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
	char *optstring = "b:a:th";
	struct option longopts[] = {
		{ "bus",         required_argument, NULL, 'b' },
		{ "address",     required_argument, NULL, 'a' },
		{ "temperature", no_argument,       NULL, 't' },
		{ "humidity",    no_argument,       NULL, 'h' },
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
				value = TEMP;
				break;
			case 'h':
				value = HUM;
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
	return fprintf(stderr, "usage: %s --bus=<bus> | -b <bus> --address=<addr> | -a <addr> [-h | --humidity | -t | --temperature]\n", name);
}

struct reading getdata()
{
	struct reading d;
	d.hum=NAN;
	d.temp=NAN;
	int f;
	if ((f = opendev(O_RDWR)) < 0) {
		return d;
	}

	/* request data conversion */
	write(f, NULL, 0);

	char *buf = (char*)malloc(8);
	if (read(f, buf, 8) < 8) {
		free(buf);
		close(f);
		return d;
	}
	convdata(buf, &d);
	free(buf);
	close(f);
	return d;
}

int convdata(char *buf, struct reading *d)
{
	d->hum = *((float *)buf);
	d->temp = *((float *)(buf+4));

	return 0;
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
