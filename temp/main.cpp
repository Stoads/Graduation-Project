#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

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

#define MAX_SOCK 2000
#define MAX_PENDING 5
#define MAX_BUFF 1024

struct client_info{
	int s;
	char ip[30];
	char id[20];
	char nickname[30];
};

MYSQL* db_conn;
MYSQL_RES* db_res;
MYSQL_ROW db_row;
struct client_info *clients[MAX_SOCK];

void* get_mysql_connect(void* arg);
int create_tcp_server_sock(FILE* _log, unsigned short port);
void message_to_client(FILE* _log,int cli_s,char* cli_ip,const char* message);
void signin(FILE* _log,int s,char* ip,char *message);
void signup(FILE * _log, int s,char* ip,char *message);


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
	socklen_t clilen;
	struct timeval set_timeout;
	if(argc<2){
		printf("server port unknown");
		return 0;
	}

	FILE* _log=open_log();
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
			clients[num_c]=new struct client_info;
			clients[num_c]->s=client_fd;
			strcpy(clients[num_c]->ip,inet_ntoa(client_addr.sin_addr));
			memset(clients[num_c]->id,0,sizeof(clients[num_c]->id));
			memset(clients[num_c]->nickname,0,sizeof(clients[num_c]->nickname));
			//thread creating
			pthread_create(cli_t+num_c,NULL,/*client function*/,(void*)clients[num_c]);

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
	mysqlID.password="awshsudb#0308";
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

void message_to_client(FILE* _log,int cli_s,char* cli_ip,const char* message){
	int n=strlen(message);
	send(cli_s,message,n,0);
	write_log(_log,3,cli_ip," message to user : ",message);
}

void signin(FILE* _log,int s,char* ip,char *message){
	/*if(*sid){
		message_to_client(_log,s,ip,"206/already signed");
		continue;
	}*/
	char* id,*pw;
	id=strtok(NULL,";");
	pw=strtok(NULL,";");
	p=strtok(NULL,";");
	if(id==NULL||pw==NULL||p!=NULL){
		message_to_client(_log,s,ip,"201/signin messag form error");
		continue;
	}
	//DB query
	char query[MAX_BUFF]="";
	sprintf(query,"SELECT id,pw,nickname FROM user WHERE id = '%s'",id);
	db_res=mysql_return_query(db_conn,query);
	if(db_res==NULL){
		message_to_client(_log,s,ip,"203/db query error occured");
		continue;
	}
	db_row=mysql_fetch_row(db_res);
	if(db_row==NULL){
		message_to_client(_log,s,ip,"208/incorrect id");
		continue;
	}
	if(strcmp(db_row[1],pw)){
		message_to_client(_log,s,ip,"209/incorrect password");
		continue;
	}
	strcpy(clients[i].id,db_row[0]);
	strcpy(clients[i].nickname,db_row[2]);
	message_to_client(_log,s,ip,"200/signin success");
}

void signup(FILE * _log, int s,char* ip,char *message){
	/*if(*sid){
		message_to_client(_log,s,ip,"206/already signed");
		continue;
	}*/
	char* id,*pw,*nickname;
	id=strtok(NULL,";");
	pw=strtok(NULL,";");
	nickname=strtok(NULL,";");
	p=strtok(NULL,";");
	if(id==NULL||pw==NULL||nickname==NULL||p!=NULL){
		message_to_client(_log,s,ip,"202/signup messag form error");
		continue;
	}
	//DB query
	char query[MAX_BUFF]="";
	//now not safe sql injection, but testing
	sprintf(query,"SELECT id From user WHERE id = '%s'",id);
	db_res=mysql_return_query(db_conn,query);
	if(db_res==NULL){
		message_to_client(_log,s,ip,"203/db query error occured");
		continue;
	}
	db_row=mysql_fetch_row(db_res);
	//same ID exists
	if(db_row!=NULL){
		message_to_client(_log,s,ip,"204/same id already exists");
		continue;
	}
	sprintf(query,"INSERT INTO user(id,pw,nickname) VALUE('%s','%s','%s')",id,pw,nickname);
	if(mysql_noreturn_query(db_conn,query)){
		message_to_client(_log,s,ip,"205/db query error occured");
		continue;
	}
	else{
		message_to_client(_log,s,ip,"200/signup success");
	}
	
}
