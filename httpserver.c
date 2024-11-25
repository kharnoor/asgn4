#include "asgn2_helper_funcs.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <regex.h>
// #include "List.h"
#include "hashmap.h"
#include "regex.h"
#include "queue.h"
#include "rwlock.h"
queue_t *q;
HashMap map;
#define BUFFER_LEN 4097
// #define command "([a-zA-Z]{1,8})"
// #define filename " (/[a-zA-Z0-9-.]{1,63})"
// #define http "(HTTP/[0-9]\\.[0-9])"
// #define status_line (command + filename + http + "\r\n")

// extern Request R;
void process_args(int argc, char *argv[], int *port, int *num_threads) {
    int opt;
    // Set default values
    *num_threads = 4;
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't': *num_threads = atoi(optarg); break;
        default: fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]); exit(EXIT_FAILURE);
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }
    *port = atoi(argv[optind]);
}

void *worker() {
    while (1) {
        // Perform the logic (process) on that connection
        char buffer[BUFFER_LEN];
        memset(buffer, 0, BUFFER_LEN);
        uintptr_t socket = 0;
        // Pop a socket from the queue
        if (queue_pop(q, (void **) &socket)) {
            struct Request R;
            memset(&R, 0, sizeof(struct Request));
            R.failed = 1;
            perform_connections(&R, socket, buffer);
            // Close the connection
            close(socket);
        }
    }
}

int main(int argc, char *argv[]) {
    int num_threads = 4;
    int port = 0;
    process_args(argc, argv, &port, &num_threads);

    // Create a listener socket for the port
    Listener_Socket socket;
    int socket_connect = listener_init(&socket, port);
    if (socket_connect == -1) {
        fprintf(stderr, "Unable to create connection for the socket\n");
        return -1;
    }
    map = *initializeHashMap();
    // Initialize the queue
    q = queue_new(num_threads);

    // Create worker threads
    pthread_t workers[num_threads];
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&workers[i], NULL, worker, NULL);
    }

    // Main loop to accept new connections
    while (1) {
        uintptr_t fd = listener_accept(&socket);
        // if (fd != -1) {
        // Push the socket onto the queue for processing by a worker thread
        queue_push(q, (void *) fd);
        // }
    }
}
