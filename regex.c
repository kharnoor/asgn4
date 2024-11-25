#include "asgn2_helper_funcs.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <regex.h>
#include "regex.h"
#include <errno.h>
#include "rwlock.h"
// #include "List.h"
#define BUFFER_LEN  4097
#define KEY_VAL_LEN 128
// extern int fd;
// int status = 200;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// List* L;
// rwlock_t *rwl;
// extern HashMap map;
void freeAll(regex_t r1, regex_t r2, regex_t r3) {
    regfree(&r1);
    regfree(&r2);
    regfree(&r3);
}

// for readability purposes
void extract_and_copy(char *dest, const char *buffer, int start, int end) {
    memcpy(dest, buffer + start, end - start);
}
// ask chatgpt to debug a memmem bc i wasnt able to include man 3 memmem properly
void *memmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len) {
    if (haystack_len < needle_len)
        return NULL;

    const unsigned char *h = (const unsigned char *) haystack;
    const unsigned char *n = (const unsigned char *) needle;
    size_t i, j;

    for (i = 0; i <= haystack_len - needle_len; ++i) {
        for (j = 0; j < needle_len; ++j) {
            if (h[i + j] != n[j])
                break;
        }
        if (j == needle_len)
            return (void *) (h + i);
    }

    return NULL;
}

int characters_after_double_newline(const char *str, char *substring, int r_u_b) {
    const char *ptr = memmem(str, r_u_b, "\r\n\r\n", 4);
    if (ptr == NULL)
        return -1; // Pattern not found
    ptr += 4; // Move past "\r\n\r\n"
    const char *end_ptr = str + r_u_b; // Pointer to the end of the string

    // Calculate the length of the substring after "\r\n\r\n"
    int length = end_ptr - ptr;
    memcpy(substring, ptr, length);
    // substring[copy_length] = '\0'; // Null-terminate the string
    return length;
}
int parse_regex(Request *R, char *buffer) {
    // set up and checks
    regex_t regex_sl;
    regex_t regex_header;
    regex_t regex_message;
    regmatch_t pmatch0[4]; // sl (all, get/put, uri, http) - request
    regmatch_t pmatch1[3]; // header (all, "content-length", content-length) - key: value
    // regmatch_t pmatch2[2]; // message body (all, body)
    regcomp(&regex_sl, "^([a-zA-Z]{1,8}) /([a-zA-Z0-9.-]{1,63}) (HTTP/[0-9][.][0-9])\r\n",
        REG_EXTENDED);
    regcomp(&regex_header, "([a-zA-Z0-9.-]{1,128}): ([ -~]{1,128})\r\n", REG_EXTENDED);
    regcomp(&regex_message, "\r\n(.*)", REG_EXTENDED);
    if (regexec(&regex_sl, buffer, 4, pmatch0, 0) == REG_NOMATCH) {
        freeAll(regex_sl, regex_header, regex_message);
        R->failed = 111;
        return -1;
    }

    // request
    extract_and_copy(R->command, buffer, pmatch0[1].rm_so, pmatch0[1].rm_eo);
    extract_and_copy(R->file, buffer, pmatch0[2].rm_so, pmatch0[2].rm_eo);
    pthread_mutex_lock(&lock);
    if (gget(&map, R->file) == NULL) {
        rwlock_t *rwl = rwlock_new(N_WAY, 1);
        insert(&map, R->file, rwl);
    }
    pthread_mutex_unlock(&lock);
    extract_and_copy(R->version, buffer, pmatch0[3].rm_so, pmatch0[3].rm_eo);

    // headers
    int header_offset = pmatch0[0].rm_eo;
    char *ptr = buffer + header_offset;
    while (regexec(&regex_header, ptr, 3, pmatch1, 0) == 0) {
        // format - key: value
        if (strcmp(ptr, ptr + pmatch1[0].rm_so) == 0) {
            int pair_offset = pmatch1[0].rm_eo;
            char curr_key[KEY_VAL_LEN + 1] = { 0 };
            memset(curr_key, '\0', KEY_VAL_LEN + 1);
            extract_and_copy(curr_key, ptr, pmatch1[1].rm_so, pmatch1[1].rm_eo);
            if (strcmp(curr_key, "Request-Id") == 0) {
                // extract_and_copy(R->key, ptr, pmatch1[1].rm_so, pmatch1[1].rm_eo);
                extract_and_copy(R->request_id, ptr, pmatch1[2].rm_so, pmatch1[2].rm_eo);
            }
            if (strcmp(curr_key, "Content-Length") == 0) {
                extract_and_copy(R->key, ptr, pmatch1[1].rm_so, pmatch1[1].rm_eo);
                extract_and_copy(R->value, ptr, pmatch1[2].rm_so, pmatch1[2].rm_eo);
            }
            ptr += pair_offset;
        } else {
            freeAll(regex_sl, regex_header, regex_message);
            return -1;
        }
    }
    // // debugged w help from brian
    // int mb = 0;
    // // message
    // if (regexec(&regex_message, ptr, 2, pmatch2, 0) == 0) {
    //     if (strcmp(ptr, ptr + pmatch2[0].rm_so) == 0) {
    //         // size of message body
    //         mb = pmatch2[1].rm_eo - pmatch2[1].rm_so;
    //         extract_and_copy(R->message_body, ptr, pmatch2[1].rm_so, pmatch2[1].rm_eo);
    //     } else {
    //         freeAll(regex_sl, regex_header, regex_message);
    //         return -1;
    //     }

    // } else {
    //     freeAll(regex_sl, regex_header, regex_message);
    //     return -1;
    // }
    freeAll(regex_sl, regex_header, regex_message);
    // return mb; // helps w pass n bytes
    return 0;
}

int get(char *location, int fd, char *buffer) {
    // rwlock variable = NULL
    pthread_mutex_lock(&lock);
    rwlock_t *rwl = gget(&map, location);
    // if (rwl == NULL) {
    //     rwl = rwlock_new(N_WAY, 1);
    //     insert(&map, location, rwl);
    // }
    reader_lock(rwl);
    pthread_mutex_unlock(&lock);
    int status = 200;
    if (chdir(location) == 0) {
        status = 403;
        // fprintf(stderr, "here\n");
        reader_unlock(rwl);
        // pthread_mutex_unlock(&lock);
        // close(file_fd);
        return status;
    }
    // fprintf(stderr, "get: before reader and pthread lock\n");

    // status = 200;
    // fprintf(stderr, "get: after reader lock. before pthread lock\n");
    // pthread_mutex_lock(&lock);
    // fprintf(stderr, "get: after reader and pthread lock\n");
    int file_fd = open(location, O_RDONLY, 0644);

    // fprintf(stderr, "errno is %d\n", errno);
    if (errno == 2) {
        status = 404;
        close(file_fd);
        // pthread_mutex_unlock(&lock);
        reader_unlock(rwl);
        return status;
    }
    struct stat statbuf;
    fstat(file_fd, &statbuf);
    uint64_t size = statbuf.st_size;
    char response[50] = { 0 };
    char size_len[20] = { 0 };
    sprintf(size_len, "%lu", size);
    // printf("%lu\n", strlen(size_len));
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n", size);
    strncpy(buffer, response, 37 + strlen(size_len));
    write_n_bytes(fd, buffer, 37 + strlen(size_len));
    pass_n_bytes(file_fd, fd, size);
    // pthread_mutex_lock(&lock);
    close(file_fd);
    reader_unlock(rwl);

    return status;
}

int put(char *location, int cl, char *message, int fd, int mb) {
    // fprintf(stdout, "%d\n", 200 - &s);

    pthread_mutex_lock(&lock);
    fprintf(stdout, "put: inside mutex lock - %s\n", location);
    rwlock_t *rwl = gget(&map, location);
    if (rwl == NULL) {
        rwl = rwlock_new(N_WAY, 1);
        insert(&map, location, rwl);
    }

    // pthread_mutex_unlock(&lock);

    // fprintf(stderr, "put: before writer and pthread lock\n");
    writer_lock(rwl);
    pthread_mutex_unlock(&lock);
    int ret = 0;
    // fprintf(stderr, "put: after writer lock. before pthread lock\n");
    // pthread_mutex_lock(&lock);
    // fprintf(stderr, "put: after writer and pthread lock\n");
    // pthread_mutex_lock(&lock);
    int check_exist_fd = open(location, O_WRONLY | O_TRUNC, 0644);
    // pthread_mutex_unlock(&lock);
    if (check_exist_fd != -1) {
        write_n_bytes(check_exist_fd, message, mb);
        // fprintf(stderr, "calling pass n bytes on cl - mb = %d. %d bytes are remaining.\n", cl - mb, mb);
        pass_n_bytes(fd, check_exist_fd, cl - mb);
        write_n_bytes(fd, "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n", 41);
        // pthread_mutex_lock(&lock);
        close(check_exist_fd);
        // pthread_mutex_unlock(&lock);
        ret = 200;
        // writer_unlock(rwl);
        // pthread_mutex_unlock(&lock);
    } else {
        // open file with creat
        // pthread_mutex_lock(&lock);
        int file_fd = open(location, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        // pthread_mutex_unlock(&lock);
        // write to file
        write_n_bytes(file_fd, message, mb);
        // fprintf(stderr, "cl: %d. mb: %d. remaining: %d. calling pass n bytes on cl - mb = %d\n", cl, mb, cl - mb, cl - mb);
        // fprintf(stderr, "source: %d, dest: %d, dest file name: %s, bytes: %d\n", fd, file_fd, location, cl - mb);
        pass_n_bytes(fd, file_fd, cl - mb);
        fprintf(stdout, "here\n");
        // created to buffer
        write_n_bytes(fd, "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n", 51);
        // pthread_mutex_lock(&lock);
        close(file_fd);
        // pthread_mutex_unlock(&lock);
        ret = 201;
        // writer_unlock(rwl);
    }
    // fprintf(stderr, "put: before writer and pthread unlock\n");
    // pthread_mutex_unlock(&lock);
    // fprintf(stderr, "put: after writer unlock. before pthread unlock\n");
    writer_unlock(rwl);
    // pthread_mutex_unlock(&lock);
    // fprintf(stderr, "put: after writer and pthread lock\n");
    return ret;
}

void perform_connections(Request *R, int fd, char *buffer) {
    // fprintf(stderr, "failed value at begginning: %d\n", R->failed);
    // fprintf(stderr, "perform connections- socket: %d\n", (int) fd);
    // pthread_mutex_lock(&lock);
    memcpy(R->request_id, "0", 1);
    // status = 200;
    R->status = 200;
    memset(buffer, 0, BUFFER_LEN);
    ssize_t read_until_bytes = read_until(fd, buffer, 2048, "\r\n\r\n");
    parse_regex(R, buffer);
    int mb = characters_after_double_newline(buffer, R->message_body, read_until_bytes);
    // pthread_mutex_unlock(&lock);
    // fprintf(stderr, "bytes read with regex: %d\n", mb);
    // fprintf(stderr, "command: %s\n", R->command);
    // fprintf(stderr, "file: %s\n", R->file);
    // fprintf(stderr, "version: %s\n", R->version);
    // fprintf(stderr, "key: %s\n", R->key);
    // fprintf(stderr, "value: %s\n", R->value);
    // fprintf(stderr, "message body: %s\n", R->message_body);
    // fprintf(stderr, "%lu\n", sizeof(R->message_body));
    if (strcmp("HTTP/1.1", R->version) != 0) {
        R->status = 400;
    }
    // fprintf(stderr, "failed value after regex sl: %d\n", R->failed);
    if (R->failed == 111) {
        R->status = 400;
    } else {
        // printf("command - %s\n", R->command);
        // printf("uri - %s\n", R->file);
        char file[strlen(R->file)];
        strcpy(file, R->file);
        // printf("version - %s\n", R->version);
        if (strcmp("HTTP/1.1", R->version) != 0) {
            R->status = 505;
        }
        if ((strcmp("GET", R->command) != 0) && (strcmp("PUT", R->command) != 0)) {
            R->status = 501;
        } else if ((strcmp("GET", R->command) == 0) && R->status == 200) {
            // fprintf(stderr, "in get, parsed header. failed value is %d\n", R->failed);
            // fprintf(stderr, "buffer: -----\n%s\n", buffer);
            // append(L, R->file);
            // pthread_mutex_lock(&lock);
            R->status = get(R->file, fd, buffer);
            // pthread_mutex_unlock(&lock);
        } else if ((strcmp("PUT", R->command) == 0) && R->status == 200) {
            if (R->failed == 111) {
                R->status = 400;
            } else {
                // printf("\n----------\n%s\n", buffer);
                if (R->failed == 111) {
                    R->status = 400;
                } else {
                    // append(L, R->file);
                    fprintf(stdout, "status before %d\n", R->status);
                    // pthread_mutex_lock(&lock);
                    R->status = put(file, atoi(R->value), R->message_body, fd, mb);
                    // pthread_mutex_unlock(&lock);
                    fprintf(stdout, "status after %d\n", R->status);
                }
            }
        }
    }
    // pthread_mutex_lock(&lock);
    // GET /foo.txt HTTP/1.1\r\n\r\nhi
    if (R->status == 505) {
        write_n_bytes(fd,
            "HTTP/1.1 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion Not "
            "Supported\n",
            80);
    } else if (R->status == 501 && strcmp("HTTP/1.1", R->version) == 0) {
        write_n_bytes(
            fd, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n", 68);
    } else if (R->status == 400 || strcmp("HTTP/1.1", R->version) != 0) {
        write_n_bytes(
            fd, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n", 60);
    } else if (R->status == 404) {
        write_n_bytes(fd, "HTTP/1.1 404 Not Found\r\nContent-Length: 12\r\n\r\nNot Found\n", 56);
    } else if (R->status == 403) {
        write_n_bytes(fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n", 56);
    }
    // fprintf(stdout, "status here: %d\n", R->status);
    // if (R->status != 201) {
    //     R->status = status;
    // }
    // fprintf(stdout, "status here: %d\n", R->status);

    fprintf(stderr, "%s,/%s,%d,%s\n", R->command, R->file, R->status, R->request_id);
    // pthread_mutex_unlock(&lock);
}
