# dwmblocks-fast
A modular status bar for window managers written in C (fork of dwmblocks).

![Alt text](./dwmblocks-fast.png?raw=true "dwmblocks-fast")
# Features
- Blocks are implemented as [C functions](#adding-a-c-function) or [shell scripts](#adding-a-shell-script).
- Only updates the statusbar when no change has occured.
- Monitors Nvidia GPU temperature, usage, and VRAM usage.
- Avoids using printf and scanf-like functions, which avoids the runtime overhead of format parsing.

# Installation
## Arch Linux
```
$ git clone https://aur.archlinux.org/packages/dwmblocks-fast-git
$ cd dwmblocks-fast-git
$ makepkg --nobuild --nodeps
```
Folow the additional instructions from makepkg.
Optionally, manually [configure](#manual) config.h, blocks.h, and the Makefile.
```
$ cd src/dwmblocks-fast/src
```
And return to the directory of the PKGBUILD.
```
$ cd ../../..
$ makepkg -si -f
```
Or for a custom configuration:
```
$ DWMBLOCKS_FAST_OPTIONS='disable-some-library' makepkg -si -f
```

# Building
Install the [dependencies](#dependencies) with your package manager. These can be
excluded with [make config](#automatic).
```
$ make config
```
Follow the additional instructions from make.
```
$ cd src
```
Optionally, manually [configure](#manual) config.h, blocks.h, and the Makefile.
```
$ cd ..
$ sudo make install
```
## Dependencies
- alsa-lib: audio monitoring
- cuda: GPU temperature monitoring with NVML
- libx11: printing to the status bar
## Optional dependencies
- dwm: window manager
- gst-plugins-base-libs: sound notifications
- dunst: popup notifications
- pamixer: audio control
- procps: send signals with pkill

# Usage
## .xinitrc (for window managers that use WM_NAME)
```
dwmblocks-fast &
```
## .xinitrc (pipewire-alsa)
For Pipewire, since the program depends on alsa-lib for audio,
Pipewire needs to be already running before dwmblocks-fast.
Otherwise, the program will fail when it tries to call
alsa-lib functions.
```
# Start Pipewire
if ps -e -o comm | grep -q -F 'systemd'; then
	systemctl --user start pipewire.service
	systemctl --user start wireplumber.service
else
	pipewire &
	wireplumber &
fi

{
	retry=5
    # Retry until Pipewire is running.
	while [ $retry -gt 0 ]; do
        # Check if pipewire is running.
		if ps -e -o comm | grep -q -F 'pipewire' && dwmblocks-fast; then
            # Success
			exit
		fi
        # Retry
		retry=$((retry - 1))
		sleep 1
	done
	exit 1 
} &
```
## Print to stdout (for other window managers that read stdin)
```
dwmblocks -p # | some_window_manager
```
# Modifying blocks
## Adding a shell script
### src/blocks.h
```
#define SIG_SH 11
static struct Block g_blocks[] = {
    /* Update_interval   Signal    Label    Function        Command*/
    { 0,                 SIG_SH,   "",      b_write_shell,  "my_shell_script" },
}
```
## Adding a C function
### src/blocks.h
```
static struct Block g_blocks[] = {
    /* Update_interval   Signal    Label    Function    Command*/
    { 2,                 0,        "",      write_my,   NULL },
}
```
### src/blocks.h
```
static char *
write_my(char *dst, unsigned int dst_len, const char *unused, unsigned int *interval)
{
    /* Do something, output to dst. */
    return dst;
}
```
## Triggering an update
```
SIG_SH=11
pkill -RTMIN+"$SIG_SH" dwmblocks-fast
```

# Configuration
To enable or disable certain features or libraries, comment them out in the config.h
and the Makefile. For example, to disable NVML:
## Automatic
Some features can be configured automatically with make (which is useful for automation).
```
$ make config # prints the available options
```
For example, to exclude CUDA from the dependencies:
```
$ make disable-cuda
```
## Manual
```
# NVML (comment to disable)
# NVMLLIB = /opt/cuda/lib64
# LDFLAGS += -L$(NVMLLIB) -lnvidia-ml
```
### Makefile
```
# NVML (comment to disable)
# NVMLLIB = /opt/cuda/lib64
# LDFLAGS += -L$(NVMLLIB) -lnvidia-ml
```
### config.h
```
/* Monitor Nvidia GPU, requires CUDA. Comment to disable. */
/* #define USE_CUDA 1 */
```
