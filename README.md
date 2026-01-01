# dwmblocks-fast
Modular status bar for dwm written in c.
# IAKOBVS's fork
This build enables the ability to choose between using C functions or shell scripts. 
# Usage
To use dwmblocks first run 'make' and then install it with 'sudo make install'.
After that you can put dwmblocks in your xinitrc or other startup script to have it start with dwm.
# Modifying blocks
The statusbar is made from text output from commandline programs or C functions in components.h.
Blocks are added and removed by editing the blocks.h header file.
By default the blocks.h header file is created the first time you run make which copies the default config from blocks.def.h.
This is so you can edit your status bar commands and they will not get overwritten in a future update.
# Dependencies
NVML (CUDA): for monitoring GPU temperature for Nvidia

alsa-lib: for monitoring audio volume

Linux: for monitoring RAM usage with sysinfo
