#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "args.h"

/*
* Deze functie parseert de door de cli meegegeven argumenten.
* Wanneer een argument als niet juist wordt herkend (of bij een andere fout), 
* zal de functie een foutcode teruggeven en een foutmelding naar stderr schrijven.
* Bij succes wordt 0 teruggegeven.
* De mogelijke argumenten zijn:
* (zie hieronder)
*/
int parse_args(int argc, char *argv[], CLIArgs *args) {
	if (!args) return -1;
	args->file = NULL;
	args->w = -1;
	args->h = -1;
	args->m = -1;

	// CLI arguments: zie HOC Slides 3c_advanced.pdf, vanaf dia 4
	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];
		if (arg[0] != '-') {
			fprintf(stderr, "Unknown argument: %s\n", arg);
			return 1;
		}

		switch (arg[1]) {
			case 'f':
			{
				if (strcmp(arg, "-f") != 0) {
					fprintf(stderr, "Unknown argument: %s\n", arg);
					return 1;
				}
				if (i + 1 < argc) {
					args->file = argv[++i];
				} else {
					fprintf(stderr, "Missing filename after -f\n");
					return 1;
				}
				break;
			}
			case 'w':
			{
				if (strcmp(arg, "-w") != 0) {
					fprintf(stderr, "Unknown argument: %s\n", arg);
					return 1;
				}
				if (i + 1 < argc) {
					args->w = atoi(argv[++i]);
				} else {
					fprintf(stderr, "Missing width after -w\n");
					return 1;
				}
				break;
			}
			case 'h':
			{
				if (strcmp(arg, "-h") != 0) {
					fprintf(stderr, "Unknown argument: %s\n", arg);
					return 1;
				}
				if (i + 1 < argc) {
					args->h = atoi(argv[++i]);
				} else {
					fprintf(stderr, "Missing height after -h\n");
					return 1;
				}
				break;
			}
			case 'm':
			{
				if (strcmp(arg, "-m") != 0) {
					fprintf(stderr, "Unknown argument: %s\n", arg);
					return 1;
				}
				if (i + 1 < argc) {
					args->m = atoi(argv[++i]);
				} else {
					fprintf(stderr, "Missing mines after -m\n");
					return 1;
				}
				break;
			}
			default:
				fprintf(stderr, "Unknown argument: %s\n", arg);
				return 1;
		}
	}

	// Je kan niet -f combineren met -w/-h/-m
	if (args->file && (args->w != -1 || args->h != -1 || args->m != -1)) {
		fprintf(stderr, "Cannot combine -f with -w/-h/-m options\n");
		return 1;
	}

	// If width/height/mines given, make sure mines does not exceed total boxes
	if (!args->file && args->w > 0 && args->h > 0 && args->m >= 0) {
		int total = (int)args->w * (int)args->h;
		if ((int)args->m > total) {
			fprintf(stderr, "Number of mines (%d) cannot exceed number of boxes (%d)\n", args->m, total);
			return 1;
		}
	}

	return 0;
}
