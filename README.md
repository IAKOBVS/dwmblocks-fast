# dwmblocks-fast
A modular status bar for dwm written in C. Being able to print to stdout, it can be trivially
made to work with other window managers.

![Alt text](./dwmblocks-fast.png?raw=true "dwmblocks-fast")
# Changes from dwmblocks
dwmblocks-fast, forked from dwmblocks, enables the user to choose between C functions, for speed,
or shell scripts. The main loop is optimized to only reconstruct the status string when there is
an actual change.
# Installation
```
make
sudo make install
```
# Usage
```
# inside ~/.xinitrc
dwmblocks-fast &
# or to print to standard output to pipe to other programs
dwmblocks -p
```
# Modifying blocks
The statusbar is made from text output from commandline programs, or C functions.
Blocks are added and removed by editing the blocks.h header file.
By default the blocks.h header file is created the first time you run make which copies
the default config from blocks.def.h.
This is so you can edit your status bar commands and they will not get overwritten in a future update.
# Dependencies
NVML (CUDA): for monitoring GPU temperature for Nvidia

alsa-lib: for monitoring audio volume

Linux: for monitoring processes with procfs and RAM usage with sysinfo
```
# for Arch Linux
pacman -S alsa-lib cuda
```
# Configuration
If you do not want to use certain configurations, not to use NVML, for example, you can
comment out the parts you want to exclude in the config.h and the Makefile, and the
program will be built without them. For example, to disable NVML:

## Makefile:
```
# NVML (comment to disable)
# NVMLLIB = /opt/cuda/lib64
# LDFLAGS += -L$(NVMLLIB) -lnvidia-ml
```

## config.h:
```
/* Monitor Nvidia GPU, requires CUDA. Comment to disable. */
/* #define USE_NVML 1 */
/* #define NVML_HEADER "/opt/cuda/targets/x86_64-linux/include/nvml.h" */
```
