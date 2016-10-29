# sdlab-sensors

SDLab sensors configuration and helper programs.
Can be used with **sdlab** project (https://github.com/robboworld/sdlab) or standalone.

This helper apps reads data from I2C sensors and shows results in stdout:

- humidicon (for HIH8000 series humidity sensors)
- sd_bh1750fvi (for BH1750FVI digital ambient light sensor)
- sdsensr (universal I2C sensors data reader)

Apps uses POSIX semaphores for concurrent access to devices on I2C bus (use `cleansem` to cleanup if have some issues).


## Build

Language: C

Get sources and compile.

```
$ cd ~/sdlab-sensors/cleansem/
$ gcc cleansem.c -lpthread -o cleansem

$ cd ~/sdlab-sensors/humidicon/
$ gcc main.c -lpthread -o humidicon

$ cd ~/sdlab-sensors/sd_bh1750fvi/
$ gcc main.c -lpthread -o sd_bh1750fvi

$ cd ~/sdlab-sensors/sdsensr/
$ gcc main.c -lpthread -o sdsensr
```


## Install

```
# cp ~/sdlab-sensors/cleansem/cleansem /usr/local/bin/
# cp ~/sdlab-sensors/humidicon/humidicon /usr/local/bin/
# cp ~/sdlab-sensors/sd_bh1750fvi/sd_bh1750fvi /usr/local/bin/
# cp ~/sdlab-sensors/sdsensr/sdsensr /usr/local/bin/

# chmod 755 /usr/local/bin/cleansem /usr/local/bin/humidicon /usr/local/bin/sd_bh1750fvi /usr/local/bin/sdsensr
```


## Install sensors configs for sdlab service

Sensors configs used only with **sdlab** project service.
Copy to location configured in **sdlab** settings.
Example:

```
# cp ~/sdlab-sensors/config/*.yml /etc/sdlab/sensors.d/
```


## Run

To get apps help, run `<app> --help`.

Example, read float32 (4 bytes) value with offset=4 bytes from device with address=0x10 on I2C bus=0:

```
# sdsensr -b 0 -a 16 -t f32 -o 4
```