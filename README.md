# dwmblocks-fast
Modular status bar for dwm written in C. Being able to print to stdout, it can be trivially
made to work with other window managers.
# IAKOBVS's fork
This build enables the ability to choose between using C functions or shell scripts.
The main loop is further optimized to lazily reconstruct the status string when there
is an actual change.
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

Linux: for monitoring RAM usage with sysinfo
```
# for Arch Linux
pacman -S alsa-lib cuda
```
# Configuration
If you do not want to use certain configurations, to use NVML, for example, you can
comment out the parts you want to
exclude in the config.h and the Makefile, and the program will be built without them.
