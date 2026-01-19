# dwmblocks-fast
A modular status bar for window managers written in C.

![Alt text](./dwmblocks-fast.png?raw=true "dwmblocks-fast")
# Changes from dwmblocks
dwmblocks-fast, forked from dwmblocks, enables the user to choose between C functions, for speed,
or shell scripts. The main loop is optimized to only reconstruct the status string when there is
an actual change.
# Installation
## Arch Linux
### Manually
```
git clone https://aur.archlinux.org/packages/dwmblocks-fast-git
cd dwmblocks-fast-git
makepkg -si
```
### Using an AUR helper
```
yay -S dwmblocks-fast-git
```
# Building
```
# your-package-manager-install alsa-lib cuda
make
sudo make install
```
# Usage
## In ~/.xinitrc (for dwm, and other window managers that use WM\_NAME)
```
dwmblocks-fast &
```
## Print to stdout (for other window managers that read stdin)
```
dwmblocks -p # | some_window_manager
```
# Modifying blocks
## Adding a shell script
### blocks.h
```
#define SIG_SH 11
static struct Block gx_blocks[] = {
    /* Update_interval   Signal    Label    Function    Command*/
    { 0,                 SIG_SH,   "",      write_cmd,  "my_shell_script" },
    /* ... */
}
```
## Adding a C functions
### blocks.h
```
static struct Block gx_blocks[] = {
    /* Update_interval   Signal    Label    Function    Command*/
    { 0,                 0,       "",       write_my,   NULL },
    /* ... */
}
```
### components.h
```
static char *
write_my (char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
    /* Do something, writing to cmd as output. */
    return dst;
}
```
## Triggering an update
```
SIG_SH=11
pkill -RTMIN+"$SIG_SH" dwmblocks-fast
```

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

## Makefile
```
# NVML (comment to disable)
# NVMLLIB = /opt/cuda/lib64
# LDFLAGS += -L$(NVMLLIB) -lnvidia-ml
```

## config.h
```
/* Monitor Nvidia GPU, requires CUDA. Comment to disable. */
/* #define USE\_NVML 1 */
/* #define NVML\_HEADER "/opt/cuda/targets/x86\_64-linux/include/nvml.h" */
```
