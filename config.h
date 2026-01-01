#ifndef CONFIG_H
#define CONFIG_H 1

#define USE_UNLOCKED_IO 1

/* Monitor Nvidia GPU, requires CUDA. Comment to disable. */
#define USE_NVML 1
#define NVML_HEADER "/opt/cuda/targets/x86_64-linux/include/nvml.h"

/* Path to CPU temperature */
#define CPU_TEMP_FILE "/sys/class/hwmon/hwmon3/temp2_input"

/* Shell scripts to execute if C functions are not available */
#define CMD_RAM_USAGE "free | awk '/^Mem:/ {printf("%d%%", 100 - ($4/$2 * 100))}'"
#define CMD_GPU_NVIDIA_TEMP "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits -i 0"
#define CMD_GPU_NVIDIA_USAGE "nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits -i 0"
#define CMD_TIME "date '+%I:%M %p'"
#define CMD_DATE "date '+%a, %d %b %Y'"
#define CMD_CPU_TEMP "head -c2 " CPU_TEMP_FILE
#define CMD_MIC_MUTED "[ $(ifmute) = 'false' ] && echo '🎤' || echo '🔇'"
#define CMD_OBS_OPEN "pgrep 'obs' > /dev/null && echo '🎥 |' || echo ''"
#define CMD_OBS_RECORDING "pgrep 'obs-ffmpeg-mux' > /dev/null && echo ' 🔴 |'"

#endif /* CONFIG_H */
