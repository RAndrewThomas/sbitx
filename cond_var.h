
void cond_init(pthread_mutex_t *mutex, pthread_cond_t *cond_var);
void cond_wait(pthread_mutex_t *mutex, pthread_cond_t *cond_var);
void cond_signal(pthread_mutex_t *mutex, pthread_cond_t *cond_var);
void cond_term(pthread_mutex_t *mutex, pthread_cond_t *cond_var);

