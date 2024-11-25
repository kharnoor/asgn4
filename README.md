# Assignment 4 directory

Multithreaded HTTP Server:

Dispatcher Thread: push connections into a queue

Worker Thread: pop the connections and perform connections on them

Get/Put: Mutex Lock around getting/creating an rwlock from the hashmap.

reader lock around critical section of get. 

writer lock around critical section of put.

sources - tutorialpoint hashamp, chatgpt, TAs - Mitchell, Pratik, Tutors - Pakhi, Johan, Sallar
