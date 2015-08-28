#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <semaphore.h>

int main()
{
	printf("removing semaphores 0 - 3\n");
	sem_unlink("i2c_bus_0_sem");
	sem_unlink("i2c_bus_1_sem");
	sem_unlink("i2c_bus_2_sem");	
	sem_unlink("i2c_bus_3_sem");
	printf("done\n");

	return 0;
}
