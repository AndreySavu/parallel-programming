#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h> 

using namespace std;
#define err_exit(code, str) { cerr << str << ": " << strerror(code) << endl; exit(EXIT_FAILURE);}

enum store_state {EMPTY, FULL} state = EMPTY;
int goods_id = 0;
pthread_mutex_t mutex;
pthread_cond_t condition;

int store;

void* producer(void* arg){
int err;
while (true) {
    err = pthread_mutex_lock(&mutex);
    if (err != 0)
        err_exit(err, "Cannot lock mutex");
    while (state == FULL) {
        err = pthread_cond_wait(&condition, &mutex);
        if (err != 0)
        err_exit(err, "Cannot wait on condition variable");
    }
    goods_id++;
    store = goods_id;
    state = FULL;
    err = pthread_cond_signal(&condition);
    if (err != 0)
        err_exit(err, "Cannot send signal");
    err = pthread_mutex_unlock(&mutex);
    if (err != 0)
        err_exit(err, "Cannot unlock mutex");
	}
}

void* consumer(void* arg)
{
int err;
while (true) {
    err = pthread_mutex_lock(&mutex);
    if (err != 0)
        err_exit(err, "Cannot lock mutex");
    while (state == EMPTY) {
        err = pthread_cond_wait(&condition, &mutex);
        if (err != 0)
            err_exit(err, "Cannot wait on condition variable");
    }
    cout << "Processing task " << store << "...";
    sleep(1);
    cout << "done" << endl;
    state = EMPTY;
    err = pthread_cond_signal(&condition);
    if (err != 0)
        err_exit(err, "Cannot send signal");
    err = pthread_mutex_unlock(&mutex);
    if (err != 0)
        err_exit(err, "Cannot unlock mutex");
    }
}

int main()
{
	int err;
	pthread_t thread1, thread2; 
	err = pthread_cond_init(&condition, NULL);
	if (err != 0)
		err_exit(err, "Cannot initialize condition variable");
	err = pthread_mutex_init(&mutex, NULL);
	if (err != 0)
		err_exit(err, "Cannot initialize mutex");
	err = pthread_create(&thread1, NULL, producer, NULL);
	if (err != 0)
		err_exit(err, "Cannot create thread 1");
	err = pthread_create(&thread2, NULL, consumer, NULL);
	if (err != 0)
		err_exit(err, "Cannot create thread 2");
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	pthread_mutex_destroy(&my_mutex);
	pthread_cond_destroy(&condition);
}
