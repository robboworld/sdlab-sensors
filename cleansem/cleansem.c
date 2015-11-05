/**
 * Clean system semaphores used for sdlab.
 * Used for unlock operations with i2c bus/devices.
 * Path: /dev/shm/
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <semaphore.h>

/*
 System Semaphores named as: i2c_bus_N_sem
 But created filenames also have prefix "sem."
 Filter by complex prefix.
*/
const char fsemprefix[] = "sem.i2c_";
const unsigned char lenpfx = 8;
const unsigned char sempfx = 4;

static int fsemfilter(const struct dirent *d)
{
	return (
#ifdef _DIRENT_HAVE_D_TYPE
		(d->d_type == DT_REG) &&
#endif
#ifdef _DIRENT_HAVE_D_NAMLEN
		(d->d_namlen >= lenpfx) &&
#else
		(strlen(d->d_name) >= lenpfx) &&
#endif
		(strncmp(d->d_name, fsemprefix, lenpfx) == 0)
	);
}

int main()
{
	struct dirent **eps;
	int n;
	int i;
	char *buf;

	puts("Removing semaphores for i2c buses...");

	n = scandir("/dev/shm/", &eps, fsemfilter, alphasort);
	if (n > 0) {
		for (i=0; i<n; ++i) {
#ifdef _DIRENT_HAVE_D_NAMLEN
			if (eps[i]->d_namlen <= sempfx) continue;
#else
			if (strlen(eps[i]->d_name) <= sempfx) continue;
#endif
			buf = strdup(eps[i]->d_name+sempfx);
			if (buf == 0) {
				perror("Out of memory");
				exit(-1);
			}
			if (sem_unlink(buf) < 0) {
				printf("Error remove %s (%d): %s\n", eps[i]->d_name, errno, strerror(errno));
			}

			free(buf);
			printf("Removed: %s\n", eps[i]->d_name);
		}
	}
	else if (n < 0) {
		perror("Couldn't open the directory");
		exit(-1);
	}

	puts("Done");

	return 0;
}
