#include <pthread.h>
#include "cond_var.h"

void cond_init(pthread_mutex_t *mutex, pthread_cond_t *cond_var) {
	pthread_mutex_init(mutex, NULL);
	pthread_cond_init(cond_var, NULL);
}

void cond_wait(pthread_mutex_t *mutex, pthread_cond_t *cond_var) {
  pthread_mutex_lock(mutex);
  pthread_cond_wait(cond_var, mutex);
  pthread_mutex_unlock(mutex);
}

void cond_signal(pthread_mutex_t *mutex, pthread_cond_t *cond_var) {
	pthread_mutex_lock(mutex);
  pthread_cond_signal(cond_var);
  pthread_mutex_unlock(mutex);
}

