#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "args.h"

/*
 * Deze functie parseert de door de cli meegegeven argumenten.
 * Wanneer een argument als niet juist wordt herkend (of bij een andere fout),
 * zal de functie een foutcode teruggeven en een foutmelding naar stderr schrijven.
 */
int parse_args(int argc, char *argv[], Args *out_args)
{
    if (!out_args)
        return -1;
    /*
     * Geef standaardwaarden aan de fields van out_args.
     * Deze struct gaat verder gebruikt worden om de geparseerde args uit te lezen.
     */
    out_args->file = NULL;
    out_args->w = -1;
    out_args->h = -1;
    out_args->m = -1;

    // CLI arguments: zie HOC Slides 3c_advanced.pdf, vanaf dia 4
    for (int i = 1; i < argc; ++i)
    {
        char *arg = argv[i];
        if (arg[0] != '-')
        {
            fprintf(stderr, "Unknown argument: %s\n", arg);
            return 1;
        }

        switch (arg[1])
        {
        case 'f': // -f <bestand>
        {
            if (strcmp(arg, "-f") != 0)
            {
                fprintf(stderr, "Unknown argument: %s\n", arg);
                return 1;
            }
            if (i + 1 < argc)
                out_args->file = argv[++i];
            else
            {
                fprintf(stderr, "Missing filename after -f\n");
                return 1;
            }
            break;
        }
        case 'w': // -w <breedte>
        {
            if (strcmp(arg, "-w") != 0)
            {
                fprintf(stderr, "Unknown argument: %s\n", arg);
                return 1;
            }
            if (i + 1 < argc)
                out_args->w = atoi(argv[++i]);
            else
            {
                fprintf(stderr, "Missing width after -w\n");
                return 1;
            }
            break;
        }
        case 'h': // -h <hoogte>
        {
            if (strcmp(arg, "-h") != 0)
            {
                fprintf(stderr, "Unknown argument: %s\n", arg);
                return 1;
            }
            if (i + 1 < argc)
                out_args->h = atoi(argv[++i]);
            else
            {
                fprintf(stderr, "Missing height after -h\n");
                return 1;
            }
            break;
        }
        case 'm': // -m <mijnen>
        {
            if (strcmp(arg, "-m") != 0)
            {
                fprintf(stderr, "Unknown argument: %s\n", arg);
                return 1;
            }
            if (i + 1 < argc)
                out_args->m = atoi(argv[++i]);
            else
            {
                fprintf(stderr, "Missing amount of mines after -m\n");
                return 1;
            }
            break;
        }
        default: // ongekend argument
            fprintf(stderr, "Unknown argument: %s\n", arg);
            return 1;
        }
    }

    // Je kan -f niet combineren met -w/-h/-m.
    if (out_args->file && (out_args->w != -1 || out_args->h != -1 || out_args->m != -1))
    {
        fprintf(stderr, "Cannot combine -f with -w/-h/-m options\n");
        return 1;
    }

    // We checken of de waarden van w, h en m geldig zijn, als er geen file wordt meegegeven.
    if (!out_args->file && out_args->w > 0 && out_args->h > 0 && out_args->m > 0)
    {
        int total = (int)out_args->w * (int)out_args->h;
        if ((int)out_args->m > total)
        {
            fprintf(stderr, "Number of mines (%d) may not exceed number of fields (%d)\n", out_args->m, total);
            return 1;
        }
    }

    return 0;
}
