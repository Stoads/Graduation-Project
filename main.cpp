#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <mysql.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include "handling.h"
#include "mysql_cont.h"
#include "child_proc.h"

#define MAX_SOCK 2000
#define MAX_PENDING 5
#define MAX_BUFF 1024

MYSQL* db_conn;
MYSQL_RES* db_res;
MYSQL_ROW db_row;
FILE* _log;
pthread_mutex_t db_mtx, log_mtx;

void* get_mysql_connect(void* arg);
int create_tcp_server_sock(FILE* _log, unsigned short port);


int main(int argc,char** argv){
	void*t_return;
	pthread_t t;
	pthread_t cli_t[MAX_SOCK];

	char recv_line[MAX_BUFF];
	fd_set read_fds;
	int running=1,num_c=0;
	int server_s, client_fd;
	int port=atoi(argv[1]);
	struct sockaddr_in client_addr;
	struct client_info *clients[MAX_SOCK];
	socklen_t clilen;
	struct timeval set_timeout;
	if(argc<2){
		printf("server port unknown");
		return 0;
	}
	pthread_mutex_init(&db_mtx,NULL);
	pthread_mutex_init(&log_mtx,NULL);

	_log=open_log();
	write_log(_log,1,"Log file opened");
	pthread_create(&t,NULL, get_mysql_connect,NULL);
	//proc arguments
	for(int i=0;i<argc;i++)
		printf("%s\n",argv[i]);
	server_s=create_tcp_server_sock(_log,port);

	pthread_join(t,&t_return);
	db_conn=(MYSQL*)t_return;
	if(db_conn==NULL)
		error_handling(_log,"Get mysql connection failed");
	write_log(_log,1,"Get mysql connection");

	while(running){
		//new client
		clilen=sizeof(client_addr);
		client_fd=accept(server_s,(struct sockaddr*)&client_addr,&clilen);
		if(client_fd!=-1){
			//remove zombie thread and fd
			for(int i=0;i<num_c;i++){
				while(clients[i]->s==-1){
					num_c--;
					free(clients[i]);
					pthread_join(cli_t[i],NULL);
					clients[i]=clients[num_c];
					cli_t[i]=cli_t[num_c];
				}
			}


			clients[num_c]=(struct client_info*)malloc(sizeof(struct client_info));
			clients[num_c]->s=client_fd;
			strcpy(clients[num_c]->ip,inet_ntoa(client_addr.sin_addr));
			memset(clients[num_c]->id,0,sizeof(clients[num_c]->id));
			memset(clients[num_c]->nickname,0,sizeof(clients[num_c]->nickname));
			//thread creating
			pthread_create(cli_t+num_c,NULL,child_proc,(void*)clients[num_c]);
			//pthread_detach(cli_t[num_c]);

			write_log(_log,2,"user connected - ",clients[num_c]->ip);
			num_c++;
		}
	}
	db_res=mysql_return_query(db_conn,"show tables");
	if(db_res==NULL)
		error_handling(_log,"Execute 'show tables' query failed");
	write_log(_log,1,"Execuete 'show tables' query");
	while((db_row=mysql_fetch_row(db_res))!=NULL)
		printf("%s\n", db_row[0]);

	mysql_free_result(db_res);
	mysql_close(db_conn);
	write_log(_log,1,"Log file closed");
	fclose(_log);
	return 0;
}


void* get_mysql_connect(void* arg){
	MYSQL* conn;
	struct connection_details mysqlID;
	mysqlID.server="capstone-db.c1guko8em5om.ap-northeast-2.rds.amazonaws.com";
	mysqlID.user="stoads";
	///password - personal info
	mysqlID.password="***";
	mysqlID.database="capstone";
	conn=mysql_connection_setup(mysqlID);
	return conn;
}

int create_tcp_server_sock(FILE* _log, unsigned short port){
	int sock;
	struct sockaddr_in serv_addr;
	char buff[MAX_BUFF]="";

	if((sock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
		error_handling(_log,"socket() failed");

	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(port);

	if(bind(sock, (struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
		error_handling(_log,"bind() failed");

	if(listen(sock, MAX_PENDING)<0)
		error_handling(_log,"listen() failed");
	sprintf(buff,"%d",port);
	write_log(_log,2,"server prepared at",buff);
	return sock;
}

