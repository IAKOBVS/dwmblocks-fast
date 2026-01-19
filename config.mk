# Compile only for this architecture (comment to disable)
CFLAGS += -march=native

# X11 (comment to disable)
LDFLAGS += -lX11

# Alsa (comment to disable)
LDFLAGS += -lasound

# NVML (comment to disable)
NVMLLIB = /opt/cuda/lib64
LDFLAGS += -L$(NVMLLIB) -lnvidia-ml

# # FreeBSD (uncomment)
#LDFLAGS += -L/usr/local/lib -I/usr/local/include

# # OpenBSD (uncomment)
# LDFLAGS += -L/usr/X11R6/lib -I/usr/X11R6/include
