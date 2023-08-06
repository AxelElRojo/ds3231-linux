# DS3231 Linux driver
Simple Linux driver for the DS3231 RTC. It uses the I²C subsystem along with the kernel RTC API. This means that the device file generated can be used as a system clock (i.e. with the hwclock command).

**While the DS3231 has two alarms that can be configured, this driver does not have alarm support.**

# Compiling
The driver was developed for and tested in Linux 5.10, different kernel versions might break it. Also, it was developed with Linaro's cross compiler for ARM version 7.5.0,
while it should work with other compilers and architechtures, it should not be overlooked.

Keep in mind that you need the kernel sources (the headers by themselves will work, but I didn't use this method) in order to compile this driver.
## Using the provided Makefile
The provided Makefile is meant for out of tree compilation, this is the method I used to develop this driver. Just change KROOT so that it points to where you store the kernel source.
Simply execute the Makefile, if you are using a cross compiler make sure you set the `ARCH` and `CROSS_COMPILE` variables to their appropiate values, this can be done in the command line, like this:
```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -j$(nproc)
```
The `-j` flag indicated the number of compilation threads to be created by make, `$(nproc)` just makes this number the same as the number of cores the host has. It makes the compilation take less time, no
that it really matters in this case, as the driver is really simple.
## In tree compilation
Do the following:
1. Add the driver to the proper directory in your kernel sources.
2. Add a Kconfig entry for the driver.
3. Add a Makefile entry for the driver, remember to use the Kconfig's name.
4. Compile the kernel or kernel modules, use the command from the previous section (or its equivalent for your system).
