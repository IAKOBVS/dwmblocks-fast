#ifndef BLOCKS_H
#	define BLOCKS_H 1

#	include "components.h"

#	define SIG_AUDIO     10
#	define SIG_OBS       9
#	define SIG_MIC       8
#	define SIG_RECORDING 7

/* sets delimeter between status commands. NULL character ('\0') means no delimeter. */
#	define DELIM    " | "
#	define DELIMLEN (S_LEN(DELIM))

struct Block {
	unsigned int interval;
	const unsigned int signal;
	const char *icon;
	char *(*func)(char *, unsigned int, const char *, unsigned int *);
	const char *command;
};

/* Modify this file to change what commands output to your statusbar, and recompile using the make command. */
static struct Block gx_blocks[] = {
	/* Set Function to write_cmd to use a shell script, Command to NULL to use a C Function */
	/* Update Interval (sec)   Signal	Label	Function	Command */
	{ 0,    SIG_OBS,   "",   write_obs_opened,        NULL },
#	ifdef USE_ALSA
	{ 0,    SIG_MIC,   "",   write_mic_muted,         NULL },
#	endif
	{ 3600, 0,         "📅", write_date,              NULL },
	{ 2,    0,         "🧠", write_ram_usage_percent, NULL },
	{ 2,    0,         "💻", write_cpu_temp,          NULL },
#	ifdef USE_NVML
	{ 2,    0,         "🚀", write_gpu_temp,          NULL },
#	endif
#	ifdef USE_ALSA
	{ 0,    SIG_AUDIO, "🔉", write_speaker_vol,       NULL },
#	endif
	{ 60,   0,         "⏰", write_time,              NULL },
};

#endif /* BLOCKS_H */
