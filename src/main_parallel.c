// For fileno() when using strict C99
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "curly.h"

static void print_usage() {
    printf("Usage: curly_parallel [options]\n");
    printf("Options:\n");
    printf("  -t, --threads N  : Number of parallel download threads (default: 4, max: 64)\n");
    printf("  -i, --input FILE : Read TSV data from FILE instead of stdin\n");
    printf("  -h, --help       : Display this help message\n");
    printf("\nInput format (TSV):\n");
    printf("  Each line should contain a URL and destination path separated by a tab:\n");
    printf("  <URL>\\t<destination_path>\\n\n");
    printf("Examples:\n");
    printf("  cat urls.tsv | curly_parallel -t 8\n");
    printf("  curly_parallel -i urls.tsv -t 16\n");
}

int main(int argc, char *argv[]) {
    int thread_count = 4;
    FILE *input_file = stdin;
    int custom_input = 0;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return EXIT_SUCCESS;
        } else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0) && i + 1 < argc) {
            thread_count = atoi(argv[i + 1]);
            if (thread_count <= 0) {
                fprintf(stderr, "Error: Thread count must be a positive integer\n");
                return EXIT_FAILURE;
            }
            i++;
        } else if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) && i + 1 < argc) {
            input_file = fopen(argv[i + 1], "r");
            if (!input_file) {
                fprintf(stderr, "Error: Cannot open input file %s\n", argv[i + 1]);
                return EXIT_FAILURE;
            }
            custom_input = 1;
            i++;
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_usage();
            return EXIT_FAILURE;
        }
    }
    
    // Check if stdin is connected to a terminal and no custom input file is provided
    if (!custom_input && isatty(fileno(stdin))) {
        fprintf(stderr, "Error: No input provided. Pipe in TSV data or use -i option.\n");
        print_usage();
        return EXIT_FAILURE;
    }
    
    // Process parallel downloads
    curly_error_t error = curly_parallel_download(thread_count, input_file);
    
    // Close input file if it's not stdin
    if (custom_input) {
        fclose(input_file);
    }
    
    if (error != CURLY_OK) {
        fprintf(stderr, "Error: %s\n", curly_strerror(error));
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}