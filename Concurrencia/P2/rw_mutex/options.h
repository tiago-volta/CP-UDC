#ifndef __OPTIONS_H__
#define __OPTIONS_H__

// Structure to store command-line options
struct options {
    int num_readers;   // Number of reader threads
    int num_writers;   // Number of writer threads
    int iterations;    // Number of iterations per thread
    int delay;         // Delay in microseconds
};

// Function to parse command-line arguments
int read_options(int argc, char **argv, struct options *opt);

#endif
