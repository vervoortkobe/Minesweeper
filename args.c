#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "args.h"

int parse_cli_args(int argc, char *argv[], CLIArgs *out) {
	if (!out) return -1;
	out->load_file = NULL;
	out->w = -1;
	out->h = -1;
	out->m = -1;
	out->error = 0;
	out->error_msg[0] = '\0';

	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];
		if (arg[0] != '-') {
			out->error = 1;
			snprintf(out->error_msg, sizeof(out->error_msg), "Unknown argument: %s", arg);
			return out->error;
		}
		switch (arg[1]) {
			case 'f':
				if (strcmp(arg, "-f") != 0) { snprintf(out->error_msg, sizeof(out->error_msg), "Unknown argument: %s", arg); out->error = 1; return out->error; }
				if (i + 1 < argc) { out->load_file = argv[++i]; } else { snprintf(out->error_msg, sizeof(out->error_msg), "Missing filename after -f"); out->error = 1; return out->error; }
				break;
			case 'w':
				if (strcmp(arg, "-w") != 0) { snprintf(out->error_msg, sizeof(out->error_msg), "Unknown argument: %s", arg); out->error = 1; return out->error; }
				if (i + 1 < argc) { out->w = atoi(argv[++i]); } else { snprintf(out->error_msg, sizeof(out->error_msg), "Missing width after -w"); out->error = 1; return out->error; }
				break;
			case 'h':
				if (strcmp(arg, "-h") != 0) { snprintf(out->error_msg, sizeof(out->error_msg), "Unknown argument: %s", arg); out->error = 1; return out->error; }
				if (i + 1 < argc) { out->h = atoi(argv[++i]); } else { snprintf(out->error_msg, sizeof(out->error_msg), "Missing height after -h"); out->error = 1; return out->error; }
				break;
			case 'm':
				if (strcmp(arg, "-m") != 0) { snprintf(out->error_msg, sizeof(out->error_msg), "Unknown argument: %s", arg); out->error = 1; return out->error; }
				if (i + 1 < argc) { out->m = atoi(argv[++i]); } else { snprintf(out->error_msg, sizeof(out->error_msg), "Missing mines after -m"); out->error = 1; return out->error; }
				break;
			default:
				snprintf(out->error_msg, sizeof(out->error_msg), "Unknown argument: %s", arg);
				out->error = 1;
				return out->error;
		}
	}

	// Je kan niet -f combineren met -w/-h/-m
	if (out->load_file && (out->w != -1 || out->h != -1 || out->m != -1)) {
		out->error = 1;
		snprintf(out->error_msg, sizeof(out->error_msg), "Cannot combine -f with -w/-h/-m options");
		return out->error;
	}

	return 0;
}
