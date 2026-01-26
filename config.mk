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
LDFLAGS_NVML += -L$(LIB_NVML) -lnvidia-ml

CFLAGS += $(CFLAGS_OPTIMIZE)
LDFLAGS += $(LDFLAGS_OPTIMIZE) $(LDFLAGS_ALSA) $(LDFLAGS_X11) $(LDFLAGS_NVML) $(LDFLAGS_FREEBSD) $(LDFLAGS_OPENBSD)
