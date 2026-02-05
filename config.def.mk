# Compile only for this architecture (comment to disable)
CFLAGS_OPTIMIZE += -march=native

# Link-time optimizations (comment to disable)
LDFLAGS_OPTIMIZE += -flto

# X11 (comment to disable)
LDFLAGS_X11 += -lX11

# Alsa (comment to disable)
LDFLAGS_ALSA += -lasound

# NVML (comment to disable)
LIB_NVML = /opt/cuda/lib64
LDFLAGS_CUDA += -L$(LIB_NVML) -lnvidia-ml

# # FreeBSD (uncomment)
# LDFLAGS_FREEBSD += -L/usr/local/lib -I/usr/local/include

# # OpenBSD (uncomment)
# LDFLAGS_OPENBSD += -L/usr/X11R6/lib -I/usr/X11R6/include
