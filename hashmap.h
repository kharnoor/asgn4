#ifndef HASHMAP_H
#define HASHMAP_H
#include <pthread.h>
#include "rwlock.h" // Include the rwlock header file

// Define a structure for key-value pair
typedef struct KeyValuePair {
    const char *key;
    rwlock_t *value;
    struct KeyValuePair *next; // Pointer to the next key-value pair
} KeyValuePair;

// Define a structure for hashmap
typedef struct HashMap {
    KeyValuePair *head; // Head of the linked list of key-value pairs
    pthread_mutex_t map_mutex;
} HashMap;

// Function prototypes

// Function to initialize the hashmap
HashMap *initializeHashMap();

// Function to insert a key-value pair into the hashmap
void insert(HashMap *map, const char *key, rwlock_t *value);

// Function to retrieve a value from the hashmap given a key
rwlock_t *gget(HashMap *map, const char *key);

#endif /* HASHMAP_H */
