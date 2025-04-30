#include "curly.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>

#define MAX_LINE_LENGTH 4096
#define DEFAULT_THREAD_COUNT 4
#define MAX_THREAD_COUNT 64

// Structure to hold a download job
typedef struct {
    char url[MAX_LINE_LENGTH];
    char destination[MAX_LINE_LENGTH];
} download_job_t;

// Structure for thread pool and job queue
typedef struct {
    download_job_t *jobs;
    size_t capacity;
    size_t size;
    size_t read_index;
    size_t write_index;
    int shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} job_queue_t;

// Thread pool context
typedef struct {
    pthread_t *threads;
    int thread_count;
    job_queue_t queue;
    int active_threads;
    pthread_mutex_t active_mutex;
} thread_pool_t;

// Global thread pool
static thread_pool_t pool;

// Function declarations for static functions
static void destroy_thread_pool(void);

// Initialize job queue
static int init_job_queue(job_queue_t *queue, size_t capacity) {
    queue->jobs = (download_job_t *)malloc(capacity * sizeof(download_job_t));
    if (!queue->jobs) {
        return -1;
    }
    
    queue->capacity = capacity;
    queue->size = 0;
    queue->read_index = 0;
    queue->write_index = 0;
    queue->shutdown = 0;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->jobs);
        return -1;
    }
    
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->jobs);
        return -1;
    }
    
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->jobs);
        return -1;
    }
    
    return 0;
}

// Destroy job queue
static void destroy_job_queue(job_queue_t *queue) {
    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = 1;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    pthread_mutex_destroy(&queue->mutex);
    free(queue->jobs);
    queue->jobs = NULL;
}

// Add job to queue
static int enqueue_job(job_queue_t *queue, const char *url, const char *destination) {
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size == queue->capacity && !queue->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    // Add job to queue
    strncpy(queue->jobs[queue->write_index].url, url, MAX_LINE_LENGTH - 1);
    queue->jobs[queue->write_index].url[MAX_LINE_LENGTH - 1] = '\0';
    
    strncpy(queue->jobs[queue->write_index].destination, destination, MAX_LINE_LENGTH - 1);
    queue->jobs[queue->write_index].destination[MAX_LINE_LENGTH - 1] = '\0';
    
    queue->write_index = (queue->write_index + 1) % queue->capacity;
    queue->size++;
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

// Get job from queue
static int dequeue_job(job_queue_t *queue, download_job_t *job) {
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    if (queue->shutdown && queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    // Get job from queue
    memcpy(job, &queue->jobs[queue->read_index], sizeof(download_job_t));
    queue->read_index = (queue->read_index + 1) % queue->capacity;
    queue->size--;
    
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

// Create directory for file if it doesn't exist
static int ensure_directory_exists(const char *path) {
    char *path_copy = strdup(path);
    if (!path_copy) {
        return -1;
    }
    
    char *dir = dirname(path_copy);
    
    // Create directories recursively
    char tmp[MAX_LINE_LENGTH];
    char *p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    
    if (len > 0 && tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    
    int result = mkdir(tmp, 0755);
    free(path_copy);
    
    // It's OK if directory already exists
    return (result == 0 || errno == EEXIST) ? 0 : -1;
}

// Callback function for writing data to a file
static size_t write_file_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    FILE *file = (FILE *)stream;
    return fwrite(ptr, size, nmemb, file);
}

// Download a file from URL to destination
curly_error_t curly_download_file(const char *url, const char *destination) {
    if (!url || !destination) {
        return CURLY_ERROR_INVALID_JSON;
    }
    
    // Make sure the directory exists
    if (ensure_directory_exists(destination) != 0) {
        return CURLY_ERROR_FILE_OPEN;
    }
    
    // Open the destination file
    FILE *file = fopen(destination, "wb");
    if (!file) {
        return CURLY_ERROR_FILE_OPEN;
    }
    
    // Initialize curl
    CURL *curl = curl_easy_init();
    if (!curl) {
        fclose(file);
        return CURLY_ERROR_CURL_INIT;
    }
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Clean up
    curl_easy_cleanup(curl);
    fclose(file);
    
    if (res != CURLE_OK) {
        // If download failed, remove the partially downloaded file
        unlink(destination);
        return CURLY_ERROR_CURL_PERFORM;
    }
    
    return CURLY_OK;
}

// Worker thread function
static void *download_worker(void *arg) {
    (void)arg; // Unused
    download_job_t job;
    
    while (1) {
        // Get a job from the queue
        if (dequeue_job(&pool.queue, &job) != 0) {
            break; // Queue is shut down
        }
        
        // Download the file
        curly_error_t result = curly_download_file(job.url, job.destination);
        
        // Print status message
        if (result == CURLY_OK) {
            printf("Downloaded %s -> %s\n", job.url, job.destination);
        } else {
            fprintf(stderr, "Failed to download %s: %s\n", job.url, curly_strerror(result));
        }
    }
    
    // Decrease active thread count
    pthread_mutex_lock(&pool.active_mutex);
    pool.active_threads--;
    pthread_mutex_unlock(&pool.active_mutex);
    
    return NULL;
}

// Initialize thread pool
static curly_error_t init_thread_pool(int thread_count) {
    if (thread_count <= 0) {
        thread_count = DEFAULT_THREAD_COUNT;
    } else if (thread_count > MAX_THREAD_COUNT) {
        thread_count = MAX_THREAD_COUNT;
    }
    
    // Initialize job queue
    if (init_job_queue(&pool.queue, thread_count * 2) != 0) {
        return CURLY_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize active threads mutex
    if (pthread_mutex_init(&pool.active_mutex, NULL) != 0) {
        destroy_job_queue(&pool.queue);
        return CURLY_ERROR_THREAD_CREATE;
    }
    
    // Allocate thread array
    pool.threads = (pthread_t *)malloc(thread_count * sizeof(pthread_t));
    if (!pool.threads) {
        destroy_job_queue(&pool.queue);
        pthread_mutex_destroy(&pool.active_mutex);
        return CURLY_ERROR_MEMORY_ALLOCATION;
    }
    
    pool.thread_count = thread_count;
    pool.active_threads = thread_count;
    
    // Create worker threads
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&pool.threads[i], NULL, download_worker, NULL) != 0) {
            // Clean up on failure
            pool.thread_count = i; // Only join threads that were created
            destroy_thread_pool();
            return CURLY_ERROR_THREAD_CREATE;
        }
    }
    
    return CURLY_OK;
}

// Destroy thread pool
static void destroy_thread_pool(void) {
    // Signal all threads to exit and wait for them
    destroy_job_queue(&pool.queue);
    
    // Join all threads
    for (int i = 0; i < pool.thread_count; i++) {
        pthread_join(pool.threads[i], NULL);
    }
    
    free(pool.threads);
    pthread_mutex_destroy(&pool.active_mutex);
}

// Parse a line of TSV data (URL, destination)
static int parse_tsv_line(char *line, char *url, char *destination) {
    char *tab = strchr(line, '\t');
    if (!tab) {
        return -1; // No tab found
    }
    
    *tab = '\0'; // Split the line at the tab
    
    // Trim trailing newline from destination
    char *newline = strchr(tab + 1, '\n');
    if (newline) {
        *newline = '\0';
    }
    
    // Copy URL and destination
    strncpy(url, line, MAX_LINE_LENGTH - 1);
    url[MAX_LINE_LENGTH - 1] = '\0';
    
    strncpy(destination, tab + 1, MAX_LINE_LENGTH - 1);
    destination[MAX_LINE_LENGTH - 1] = '\0';
    
    return 0;
}

// Process parallel downloads from TSV input
curly_error_t curly_parallel_download(int thread_count, FILE *input_stream) {
    if (!input_stream) {
        return CURLY_ERROR_UNKNOWN;
    }
    
    // Initialize curl global
    curl_global_init(CURL_GLOBAL_ALL);
    
    // Initialize thread pool
    curly_error_t result = init_thread_pool(thread_count);
    if (result != CURLY_OK) {
        curl_global_cleanup();
        return result;
    }
    
    // Read TSV data from input stream
    char line[MAX_LINE_LENGTH];
    char url[MAX_LINE_LENGTH];
    char destination[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), input_stream)) {
        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\0') {
            continue;
        }
        
        // Parse TSV line
        if (parse_tsv_line(line, url, destination) != 0) {
            fprintf(stderr, "Invalid input line: %s\n", line);
            continue;
        }
        
        // Add download job to queue
        enqueue_job(&pool.queue, url, destination);
    }
    
    // Wait for all jobs to complete
    pthread_mutex_lock(&pool.queue.mutex);
    pool.queue.shutdown = 1;
    pthread_cond_broadcast(&pool.queue.not_empty);
    pthread_mutex_unlock(&pool.queue.mutex);
    
    // Wait for all threads to finish
    int active;
    do {
        sleep(1); // Check every second
        pthread_mutex_lock(&pool.active_mutex);
        active = pool.active_threads;
        pthread_mutex_unlock(&pool.active_mutex);
    } while (active > 0);
    
    // Clean up
    destroy_thread_pool();
    curl_global_cleanup();
    
    return CURLY_OK;
}