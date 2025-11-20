#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "args.h"

// Deze functie parseert de door de cli meegegeven argumenten.
int parse_args(int argc, char *argv[], CLIArgs *args) {
	if (!args) return -1;
	args->load_file = NULL;
	args->w = -1;
	args->h = -1;
	args->m = -1;
	args->error = 0;
	args->error_msg[0] = '\0';

	// CLI arguments: zie HOC Slides 3c_advanced.pdf, vanaf dia 4
	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];
		if (arg[0] != '-') {
			args->error = 1;
			snprintf(args->error_msg, sizeof(args->error_msg), "Unknown argument: %s", arg);
			return args->error;
		}
		switch (arg[1]) {
			case 'f':
				if (strcmp(arg, "-f") != 0) { snprintf(args->error_msg, sizeof(args->error_msg), "Unknown argument: %s", arg); args->error = 1; return args->error; }
				if (i + 1 < argc) { args->load_file = argv[++i]; } else { snprintf(args->error_msg, sizeof(args->error_msg), "Missing filename after -f"); args->error = 1; return args->error; }
				break;
			case 'w':
				if (strcmp(arg, "-w") != 0) { snprintf(args->error_msg, sizeof(args->error_msg), "Unknown argument: %s", arg); args->error = 1; return args->error; }
				if (i + 1 < argc) { args->w = atoi(argv[++i]); } else { snprintf(args->error_msg, sizeof(args->error_msg), "Missing width after -w"); args->error = 1; return args->error; }
				break;
			case 'h':
				if (strcmp(arg, "-h") != 0) { snprintf(args->error_msg, sizeof(args->error_msg), "Unknown argument: %s", arg); args->error = 1; return args->error; }
				if (i + 1 < argc) { args->h = atoi(argv[++i]); } else { snprintf(args->error_msg, sizeof(args->error_msg), "Missing height after -h"); args->error = 1; return args->error; }
				break;
			case 'm':
				if (strcmp(arg, "-m") != 0) { snprintf(args->error_msg, sizeof(args->error_msg), "Unknown argument: %s", arg); args->error = 1; return args->error; }
				if (i + 1 < argc) { args->m = atoi(argv[++i]); } else { snprintf(args->error_msg, sizeof(args->error_msg), "Missing mines after -m"); args->error = 1; return args->error; }
				break;
			default:
				snprintf(args->error_msg, sizeof(args->error_msg), "Unknown argument: %s", arg);
				args->error = 1;
				return args->error;
		}
	}

	// Je kan niet -f combineren met -w/-h/-m
	if (args->load_file && (args->w != -1 || args->h != -1 || args->m != -1)) {
		args->error = 1;
		snprintf(args->error_msg, sizeof(args->error_msg), "Cannot combine -f with -w/-h/-m options");
		return args->error;
	}

	// If width/height/mines given, make sure mines does not exceed total boxes
	if (!args->load_file && args->w > 0 && args->h > 0 && args->m >= 0) {
		long total = (long)args->w * (long)args->h;
		if ((long)args->m > total) {
			args->error = 1;
			snprintf(args->error_msg, sizeof(args->error_msg), "Number of mines (%d) cannot exceed number of boxes (%ld)", args->m, total);
			return args->error;
		}
	}

	return 0;
}
