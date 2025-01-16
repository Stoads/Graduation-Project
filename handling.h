#ifndef __HANDLING_H__
#define __HANDLING_H__
//#include <pthread.h>
extern pthread_mutex_t log_mtx;
void error_handling(FILE *_log,const char* str);
void write_log(FILE* _log,int num,...);
FILE* open_log();
#endif//__HANDLING_H__
