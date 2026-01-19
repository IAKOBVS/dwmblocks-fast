# Compile only for this architecture (comment to disable)
ARCHFLAGS += -march=native

# X11 (comment to disable)
X11FLAGS += -lX11

# Alsa (comment to disable)
ALSAFLAGS += -lasound

# NVML (comment to disable)
NVMLLIB = /opt/cuda/lib64
NVMLFLAGS += -L$(NVMLLIB) -lnvidia-ml

# # FreeBSD (uncomment)
# FREEBSDFLAGS += -L/usr/local/lib -I/usr/local/include

# # OpenBSD (uncomment)
# OPENBSDFLAGS += -L/usr/X11R6/lib -I/usr/X11R6/include

CFLAGS += $(ARCHFLAGS)
LDFLAGS += $(ALSAFLAGS) $(X11FLAGS) $(NVMLFLAGS) $(FREEBSDFLAGS) $(OPENBSDFLAGS)
