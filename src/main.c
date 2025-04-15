#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curly.h"

#define MAX_JSON_SIZE 4096

static void print_usage() {
    printf("Usage: curly [options] <json_file | json_string>\n");
    printf("Options:\n");
    printf("  -f, --file     : Treat input as a file path\n");
    printf("  -s, --string   : Treat input as a JSON string\n");
    printf("  -h, --help     : Display this help message\n");
    printf("\nExamples:\n");
    printf("  curly -f request.json\n");
    printf("  curly -s '{\"url\":\"https://httpbin.org/get\"}'\n");
}

static char *read_file(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error: Unable to open file %s\n", filepath);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > MAX_JSON_SIZE) {
        fprintf(stderr, "Error: File size is invalid or too large\n");
        fclose(file);
        return NULL;
    }
    
    // Allocate buffer and read file
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';
    fclose(file);
    
    if (read_size != (size_t)file_size) {
        fprintf(stderr, "Error: Failed to read entire file\n");
        free(buffer);
        return NULL;
    }
    
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }
    
    int is_file = 0;
    char *input = NULL;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) {
            is_file = 1;
            if (i + 1 < argc) {
                input = argv[i + 1];
                i++;
            }
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--string") == 0) {
            is_file = 0;
            if (i + 1 < argc) {
                input = argv[i + 1];
                i++;
            }
        } else if (input == NULL) {
            // Default to treating as a file if no option specified
            is_file = 1;
            input = argv[i];
        }
    }
    
    // Validate input
    if (input == NULL) {
        fprintf(stderr, "Error: No input provided\n");
        print_usage();
        return EXIT_FAILURE;
    }
    
    char *json_str = NULL;
    
    if (is_file) {
        json_str = read_file(input);
        if (!json_str) {
            return EXIT_FAILURE;
        }
    } else {
        // Input is already a JSON string
        size_t len = strlen(input) + 1;
        json_str = malloc(len);
        if (!json_str) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return EXIT_FAILURE;
        }
        memcpy(json_str, input, len);
    }
    
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    
    curly_config_t config;
    curly_response_t response;
    
    // Parse JSON config
    curly_error_t error = curly_parse_config(json_str, &config);
    if (error != CURLY_OK) {
        fprintf(stderr, "Error: %s\n", curly_strerror(error));
        free(json_str);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    // Perform the request
    error = curly_perform_request(&config, &response);
    if (error != CURLY_OK) {
        fprintf(stderr, "Error: %s\n", curly_strerror(error));
        curly_free_config(&config);
        free(json_str);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    // Print the response
    printf("%s\n", response.data);
    
    // Clean up
    curly_free_config(&config);
    curly_free_response(&response);
    free(json_str);
    curl_global_cleanup();
    
    return EXIT_SUCCESS;
}
