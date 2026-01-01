#ifndef GPU_H
#	define GPU_H 1

#	include "config.h"

#	ifdef USE_NVML
#		ifndef NVML_HEADER
#			define NVML_HEADER "/opt/cuda/targets/x86_64-linux/include/nvml.h"
#		endif
#		include NVML_HEADER
#	endif

#endif /* GPU_H */
