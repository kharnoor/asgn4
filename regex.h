#pragma once
#define BUFFER_LEN  4097
#define KEY_VAL_LEN 128
#include <pthread.h>
#include "rwlock.h"
#include "queue.h"
// #include "List.h"
#include "hashmap.h"
extern pthread_mutex_t lock; // global mutex to synchronize
// extern rwlock_t *rwl;
extern HashMap map;
// extern List RWL;
extern queue_t *q;

typedef struct Request {
    char command[9]; // at most eight (8) characters from the character range [a-zA-Z] (GET or PUT)
    char file
        [64]; // 2-64 characters (including the ‘/’), only includes characters from the character set [a-zAZ0-9.-]
    char version[9]; // HTTP/1.1
    // after this is \r\n
    int status;
    char request_id[5];
    char key[KEY_VAL_LEN + 1]; // optional
    char value[KEY_VAL_LEN + 1]; // optional
    char message_body[BUFFER_LEN]; // optional (only for PUT)
    char *status_line;
    int failed;
} Request;

int parse_regex(Request *R, char *buffer);
void perform_connections(Request *R, int fd, char *buffer);
