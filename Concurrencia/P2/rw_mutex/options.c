#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "options.h"

// Define long and short command-line options
static struct option long_options[] = {
    { .name = "readers",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 'r'},
    { .name = "writers",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 'w'},
    { .name = "iterations",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 'i'},
    { .name = "delay",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 'd'},
    { .name = "help",
      .has_arg = no_argument,
      .flag = NULL,
      .val = 'h'},
    {0, 0, 0, 0}
};

// Print usage information
static void usage(int i)
{
    printf(
        "Usage:  swap [OPTION]\n"
        "Options:\n"
           "  -r n, --readers=<n>    Number of reader threads\n"
           "  -w n, --writers=<n>    Number of writer threads\n"
           "  -i n, --iterations=<n> Number of iterations per thread\n"
           "  -d n, --delay=<n>      Delay between operations (in µs)\n"
           "  -h, --help             Show this message\n\n"
    );
    exit(i);
}

// Convert argument to integer safely
static int get_int(char *arg, int *value)
{
    char *end;
    *value = strtol(arg, &end, 10);

    return (end != NULL);
}

// Handle command-line arguments
int handle_options(int argc, char **argv, struct options *opt)
{
    while (1) {
        int c;
        int option_index = 0;

        c = getopt_long (argc, argv, "r:w:i:d:h",
                 long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'r':
            if (!get_int(optarg, &opt->num_readers)
                || opt->num_readers <= 0) {
                printf("'%s': is not a valid integer\n",
                       optarg);
                usage(-3);
            }
            break;

        case 'w':
            if (!get_int(optarg, &opt->num_writers)
                || opt->num_writers <= 0) {
                printf("'%s': is not a valid integer\n",
                       optarg);
                usage(-3);
            }
            break;

        case 'i':
            if (!get_int(optarg, &opt->iterations)
                || opt->iterations <= 0) {
                printf("'%s': is not a valid integer\n",
                       optarg);
                usage(-3);
            }
            break;

        case 'd':
            if (!get_int(optarg, &opt->delay)
                || opt->delay <= 0) {
                printf("'%s': is not a valid integer\n",
                       optarg);
                usage(-3);
            }
            break;

        case '?':
        case 'h':
            usage(0);
            break;

        default:
            printf ("?? getopt returned character code 0%o ??\n", c);
            usage(-1);
        }
    }
    return 0;
}

// Read and parse command-line options
int read_options(int argc, char **argv, struct options *opt) {

    int result = handle_options(argc,argv,opt);

    if (result != 0)
        exit(result);

    if (argc - optind != 0) {
        printf ("Too many arguments\n\n");
        while (optind < argc)
            printf ("'%s' ", argv[optind++]);
        printf ("\n");
        usage(-2);
    }

    return 0;
}
