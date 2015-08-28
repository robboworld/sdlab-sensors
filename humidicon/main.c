#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <semaphore.h>

const int TIMEOUT = 1000;

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
		sem_post(smph);
		sem_close(smph);
		free(SEM_NAME);
		exit(-1);
		break;
	}

	sem_post(smph);
	sem_close(smph);
	free(SEM_NAME);

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

	char *buf = (char*)malloc(4);
	int state;
	do {
		usleep(TIMEOUT);
		if (read(f, buf, 4) < 4) {
			free(buf);
			close(f);
			return d;
		}
		state = buf[0] >> 6;
	} while (state == 0x1);
	switch (state) {
	case 0x0: /* ready */
		convdata(buf, &d);
		break;
	case 0x2: /* command mode; not described in datasheet, do nothing */
		fputs("error: device is in command mode\n", stderr);
		break;
	default: /* normally unreachable */
		fprintf(stderr, "error: device reported state %d\n", state);
		break;
	}
	free(buf);
	close(f);
	return d;
}

int convdata(char *buf, struct reading *d)
{
	int rawhum;
	int rawtemp;
	rawhum = (((int)buf[0] << 8) | (int)buf[1]) & 0x3fff;
	rawtemp = ((int)buf[2] << 6) | ((int)buf[3] >> 2);
	d->hum = (double)rawhum / 0x3ffe * 100;
	d->temp = (double)rawtemp / 0x3ffe * 165 + 233.15;
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
