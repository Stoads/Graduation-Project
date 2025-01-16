#ifndef __CHILD_PROC_H__
#define __CHILD_PROC_H__
struct client_info{
	int s;
	char ip[30];
	char id[20];
	char nickname[30];
};
void* child_proc(void* arg);//for thread
#endif//__CHILD_PROC_H__
