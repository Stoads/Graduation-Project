#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdarg>

#include <pthread.h>

#include "handling.h"

#define DATE_FORMAT "%04Y%02m%02d%02H%02M%02S"
#define STRFTIME(buff,str) strftime(buff,sizeof(buff),str,get_time_info())
#ifndef BUFF_SIZE
#define BUFF_SIZE 1024
#endif//BUFF_SIZE

struct tm* get_time_info(){
	time_t raw_time;
	time(&raw_time);
	return localtime(&raw_time);
}

void error_handling(FILE* _log, const char* message){
	fputs(message,stderr);
	fputc('\n',stderr);
	if(_log){
		write_log(_log,2,"Error : ",message);
		fclose(_log);
	}
	exit(1);
}
void write_log(FILE* _log, int num, ...){
	va_list valist;
	va_start(valist,num);

	if(_log==NULL)return;

	char time_buff[32]="";
	char cont_buff[BUFF_SIZE]="";
	STRFTIME(time_buff,"[" DATE_FORMAT "] - ");
	
	pthread_mutex_lock(&log_mtx);
	fprintf(_log,"%s",time_buff);
	while(num--){
		strcpy(cont_buff,va_arg(valist,char*));
		fprintf(_log,"%s",cont_buff);
	}
	fputc('\n',_log);
	fflush(_log);
	pthread_mutex_unlock(&log_mtx);

	va_end(valist);
}
FILE* open_log(){
	char location[99]="";
	STRFTIME(location,"log/%04Y%02m%02d");
	return fopen(location,"a+t");
}
