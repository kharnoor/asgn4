#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "hashmap.h"

// // Define a structure for key-value pair
// typedef struct KeyValuePair {
//     const char *key;
//     rwlock_t *value;
//     struct KeyValuePair *next; // Pointer to the next key-value pair
// } KeyValuePair;

// // Define a structure for hashmap
// typedef struct HashMap {
//     KeyValuePair *head; // Head of the linked list of key-value pairs
//     pthread_mutex_t map_mutex;
// } HashMap;

// Function to create a new key-value pair
KeyValuePair *createKeyValuePair(const char *key, rwlock_t *value) {
    KeyValuePair *pair = (KeyValuePair *) malloc(sizeof(KeyValuePair));
    if (pair != NULL) {
        pair->key = strdup(key); // Duplicate the key string
        pair->value = value;
        pair->next = NULL;
    }
    return pair;
}

// Function to initialize the hashmap
HashMap *initializeHashMap() {
    HashMap *map = (HashMap *) malloc(sizeof(HashMap));
    if (map != NULL) {
        map->head = NULL;
        pthread_mutex_init(&map->map_mutex, NULL);
    }
    return map;
}

// Function to insert a key-value pair into the hashmap
void insert(HashMap *map, const char *key, rwlock_t *value) {
    // pthread_mutex_lock(&map->map_mutex); // Lock before accessing the HashMap

    KeyValuePair *pair = createKeyValuePair(key, value);
    if (pair == NULL) {
        // pthread_mutex_unlock(&map->map_mutex); // Unlock if memory allocation fails
        return;
    }

    // Append the new pair to the head of the list
    pair->next = map->head;
    map->head = pair;

    // pthread_mutex_unlock(&map->map_mutex); // Unlock after accessing the HashMap
}

// Function to retrieve a value from the hashmap given a key
rwlock_t *gget(HashMap *map, const char *key) {
    // pthread_mutex_lock(&map->map_mutex); // Lock before accessing the HashMap

    KeyValuePair *current = map->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            // pthread_mutex_unlock(&map->map_mutex); // Unlock after accessing the HashMap
            return current->value;
        }
        current = current->next;
    }

    // pthread_mutex_unlock(&map->map_mutex); // Unlock after accessing the HashMap
    return NULL; // Return NULL if key not found
}

// Other functions using the HashMap should be similarly protected using the mutex.
