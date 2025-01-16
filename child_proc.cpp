#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <mysql.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "child_proc.h"
#include "mysql_cont.h"
#include "handling.h"

#ifndef BUFF_SIZE
#define BUFF_SIZE 1024
#endif//BUFF_SIZE

#define EDITABLE 2
#define READABLE 1

extern MYSQL* db_conn;
extern MYSQL_RES* db_res;
extern MYSQL_ROW db_row;
extern pthread_mutex_t db_mtx;
extern FILE* _log;

void message_to_client(FILE* _log,struct client_info *cli,const char* message,int n);
int message_from_client(FILE* _log,struct client_info *cli,char* message);
void signin(FILE* _log,struct client_info *cli,char* next_ptr);
void signup(FILE* _log,struct client_info *cli,char* next_ptr);
void signout(FILE* _log,struct client_info *cli,char* next_ptr);
void upload(FILE* _log,struct client_info *cli,char* next_ptr);
void update(FILE* _log,struct client_info *cli,char* next_ptr);
void download(FILE* _log,struct client_info *cli,char* next_ptr);
void usersearch(FILE* _log,struct client_info *cli,char* next_ptr);
void befriend(FILE* _log,struct client_info *cli,char* next_ptr);
void friendlist(FILE* _log,struct client_info *cli,char* next_ptr);
void wantbefriend(FILE* _log,struct client_info *cli,char* next_ptr);
void toshare(FILE* _log,struct client_info *cli,char* next_ptr);

void* child_proc(void* argc){
	struct client_info* cli=(struct client_info*)argc;
	char message[BUFF_SIZE]="",temp[BUFF_SIZE];
	char *type,*next_ptr;
	int len;
	for(int running=1;running;){
		len=message_from_client(_log,cli,message);
		if(len==0){	//client disconnected
			write_log(_log,2,cli->ip," is disconnected...");
			cli->s=-1;	//notice thread end main thread
			break;
		}
		strcpy(temp,message);
		if(len>1&&temp[len-1]=='\n')temp[len-1]=0;
		type=strtok_r(temp,";",&next_ptr);
		if(!strcmp(type,"signin")){
			signin(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"signup")){
			signup(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"signout")){
			signout(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"upload")){
			upload(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"update")){
			update(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"download")){
			download(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"usersearch")){
			usersearch(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"befriend")){
			befriend(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"friendlist")){
			friendlist(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"wantbefriend")){
			wantbefriend(_log,cli,next_ptr);
		}
		else if(!strcmp(type,"toshare")){
			toshare(_log,cli,next_ptr);
		}
		else{
			message_to_client(_log,cli,message,len);//echo
		}
	}
	return NULL;
}

void message_to_client(FILE* _log,struct client_info *cli,const char* message,int n=0){
	if(n==0)n=strlen(message);
	send(cli->s,message,n,0);
	write_log(_log,3,cli->ip," message to user : ",message);
}

int message_from_client(FILE* _log,struct client_info *cli,char* message){
	int n=recv(cli->s,message,BUFF_SIZE,0);
	if(n>0){
		message[n]=0;
		write_log(_log,3,cli->ip," message from user : ",message);
	}
	return n;
}

void signin(FILE* _log,struct client_info *cli,char* next_ptr){
	if(*cli->id){
		message_to_client(_log,cli,"101;already signined");
		return;
	}
	char *id=strtok_r(NULL,";",&next_ptr);
	char *pw=strtok_r(NULL,";",&next_ptr);
	if(id==NULL||pw==NULL||strtok_r(NULL,";",&next_ptr)!=NULL){
		message_to_client(_log,cli,"102;signin sentence error");
		return;
	}
	char query[BUFF_SIZE]="";
	sprintf(query,"SELECT id,pw,nickname FROM user WHERE id = '%s'",id);
	pthread_mutex_lock(&db_mtx);
	db_res=mysql_return_query(db_conn,query);
	if(db_res==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"103;select failed");
		return;
	}
	write_log(_log,2,"Execuete query : ",query);
	if((db_row=mysql_fetch_row(db_res))==NULL||strcmp(db_row[1],pw)){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"104;wrong id or password");
		return;
	}
	strcpy(cli->id,db_row[0]);
	strcpy(cli->nickname,db_row[2]);
	pthread_mutex_unlock(&db_mtx);
	char buff[BUFF_SIZE]="";
	int len=sprintf(buff,"100;%s;%s",cli->id,cli->nickname);
	message_to_client(_log,cli,buff,len);
}

void signup(FILE* _log,struct client_info *cli,char* next_ptr){
	if(*cli->id){
		message_to_client(_log,cli,"201;already signined");
		return;
	}
	char *id=strtok_r(NULL,";",&next_ptr);
	char *pw=strtok_r(NULL,";",&next_ptr);
	char *nickname=strtok_r(NULL,":",&next_ptr);
	if(id==NULL||pw==NULL||nickname==NULL||strtok_r(NULL,";",&next_ptr)!=NULL){
		message_to_client(_log,cli,"202;signup sentence error");
		return;
	}
	char query1[BUFF_SIZE]="",query2[BUFF_SIZE]="";
	sprintf(query1,"SELECT id FROM user WHERE id = '%s'",id);
	sprintf(query2,"INSERT INTO user(id,pw,nickname) VALUE('%s','%s','%s')",id,pw,nickname);
	pthread_mutex_lock(&db_mtx);
	write_log(_log,2,"Execuete query : ",query1);
	if((db_res=mysql_return_query(db_conn,query1))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"203;select failed");
		return;
	}
	db_row=mysql_fetch_row(db_res);
	if(db_row!=NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"204;same id already exists");
		return;
	}
	write_log(_log,2,"Execuete query : ",query2);
	if(mysql_noreturn_query(db_conn,query2)){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"205;insert failed");
		return;
	}
	pthread_mutex_unlock(&db_mtx);
	message_to_client(_log,cli,"200;signup success");
}

void signout(FILE* _log,struct client_info *cli,char* next_ptr){
	if(!*cli->id){
		message_to_client(_log,cli,"301;not signed");
		return;
	}
	memset(cli->id,0,sizeof(cli->id));
	memset(cli->nickname,0,sizeof(cli->nickname));
	message_to_client(_log,cli,"300;signout success");
}

void upload(FILE* _log,struct client_info *cli,char* next_ptr){
	if(!*cli->id){//have not session
		message_to_client(_log,cli,"401;not signed");
		return;
	}
	int len;
	char* arg=strtok_r(NULL,";",&next_ptr);
	char buff[BUFF_SIZE]="";
	int f_size=atoi(arg);
	if(arg==NULL||f_size==0||strtok_r(NULL,";",&next_ptr)!=NULL){
		message_to_client(_log,cli,"402;upload sentence error");
		return;
	}
	//create version, create diary_id
	char version[50]="";
	int diary_id;
	char query[BUFF_SIZE]="SELECT TIMESTAMP(now())";
	pthread_mutex_lock(&db_mtx);
	write_log(_log,2,"Execuete query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL||(db_row=mysql_fetch_row(db_res))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"403;version creating failed");
		return;
	}
	strcpy(version,db_row[0]);

	sprintf(query,"SELECT MAX(id) FROM diary");
	write_log(_log,2,"Execuete query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL||(db_row=mysql_fetch_row(db_res))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"404;diary id creating failed");
		return;
	}
	diary_id=atoi(db_row[0])+1;
	
	char loc[256]="";
	sprintf(loc,"file/%d-%s.xml",diary_id,version);
	FILE* fp=fopen(loc,"wb");
	if(fp==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"405;file create failed");
	}

	sprintf(query,"INSERT INTO diary(date, link) VALUES('%s', '%s')",version,loc);
	write_log(_log,2,"Execuete query : ",query);
	if(mysql_noreturn_query(db_conn,query)){
		pthread_mutex_unlock(&db_mtx);
		fclose(fp);
		message_to_client(_log,cli,"406;diary input failed");
		return;
	}
	
	sprintf(query,"INSERT INTO diary_history(id, version, link) VALUES('%d', '%s', '%s')",diary_id,version,loc);
	write_log(_log,2,"Execuete query : ",query);
	if(mysql_noreturn_query(db_conn,query)){
		pthread_mutex_unlock(&db_mtx);
		fclose(fp);
		message_to_client(_log,cli,"407;diary_history create failed");
		return;
	}
	
	sprintf(query,"INSERT INTO authority(user_id,diary_id,authority) VALUES('%s', '%d', '65535')",cli->id,diary_id);
	write_log(_log,2,"Execuete query : ",query);
	if(mysql_noreturn_query(db_conn,query)){
		pthread_mutex_unlock(&db_mtx);
		fclose(fp);
		message_to_client(_log,cli,"408;authority create failed");
		return;
	}
	pthread_mutex_unlock(&db_mtx);
	
	len=sprintf(buff,"400;%d;%s",diary_id,version);
	message_to_client(_log,cli,buff,len);
	for(int i=0;i<f_size&&len;i+=len){
		len=recv(cli->s,buff,BUFF_SIZE,0);
		fwrite(buff,1,len,fp);
	}
	fclose(fp);
}
void update(FILE* _log,struct client_info *cli,char* next_ptr){
	if(!*cli->id){//have not session
		message_to_client(_log,cli,"501;not signed");
		return;
	}
	
	char* diary_id_str=strtok_r(NULL,";",&next_ptr);
	char* pversion=strtok_r(NULL,";",&next_ptr);
	char* f_size_str=strtok_r(NULL,";",&next_ptr);
	int diary_id=atoi(diary_id_str);
	int f_size=atoi(f_size_str);
	if(diary_id_str==NULL||pversion==NULL||f_size_str==NULL||diary_id==0||f_size==0||strtok_r(NULL,";",&next_ptr)!=NULL){
		message_to_client(_log,cli,"502;upload sentence error");
		return;
	}
	//create version
	char version[50]="";
	char query[BUFF_SIZE]="SELECT TIMESTAMP(now())";
	pthread_mutex_lock(&db_mtx);
	write_log(_log,2,"Execuete query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL||(db_row=mysql_fetch_row(db_res))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"503;version creating failed");
		return;
	}
	strcpy(version,db_row[0]);	
	
	//latest version check
	sprintf(query,"SELECT date FROM diary WHERE id = '%d'",diary_id);	
	write_log(_log,2,"Execuete query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL||(db_row=mysql_fetch_row(db_res))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"504;wrong diary id");
		return;
	}
	if(strcmp(db_row[0],pversion)){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"505;update exists");
		return;
	}
	
	//authority check
	sprintf(query,"SELECT authority FROM authority WHERE user_id = '%s' AND diary_id = '%d'",cli->id,diary_id);
	write_log(_log,2,"Execuete query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL||(db_row=mysql_fetch_row(db_res))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"506;authority not found");
		return;
	}
	int authority=atoi(db_row[0]);
	if(0==authority&EDITABLE){	//not editable
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"507;cannot editable");
		return;
	}

	char loc[256]="";
	sprintf(loc,"file/%d-%s.xml",diary_id,version);
	FILE *fp=fopen(loc,"wb");
	if(fp==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"508;cannnot open file");
		return;
	}
	
	sprintf(query,"UPDATE diary SET date='%s', link='%s' WHERE id='%d'",version,loc,diary_id);
	write_log(_log,2,"Execuete query : ",query);
	if(mysql_noreturn_query(db_conn,query)){
		pthread_mutex_unlock(&db_mtx);
		fclose(fp);
		message_to_client(_log,cli,"509;diary table update failed");
		return;
	}
	
	sprintf(query,"INSERT INTO diary_history(id, version, link) VALUES('%d','%s','%s')",diary_id,version,loc);
	write_log(_log,2,"Execuete query : ",query);
	if(mysql_noreturn_query(db_conn,query)){
		pthread_mutex_unlock(&db_mtx);
		fclose(fp);
		message_to_client(_log,cli,"510;diary history insert failed");
		return;
	}
	pthread_mutex_unlock(&db_mtx);

	char buff[BUFF_SIZE]="";
	int len=sprintf(buff,"500;%d;%s",diary_id,version);
	message_to_client(_log,cli,buff,len);
	for(int i=0;i<f_size&&len;i+=len){
		len=recv(cli->s,buff,BUFF_SIZE,0);
		fwrite(buff,1,len,fp);
	}
	fclose(fp);
}
void download(FILE* _log,struct client_info *cli,char* next_ptr){
	if(!*cli->id){//have not session
		message_to_client(_log,cli,"601;not signed");
		return;
	}
	char* diary_id_str=strtok_r(NULL,";",&next_ptr);
	int diary_id=atoi(diary_id_str);
	if(diary_id_str==NULL||diary_id==0||strtok_r(NULL,";",&next_ptr)!=NULL){
		message_to_client(_log,cli,"602;download sentence error");
		return;
	}
	char query[BUFF_SIZE]="";
	sprintf(query,"SELECT authority FROM authority WHERE user_id='%s' AND diary_id='%d'",cli->id,diary_id);
	pthread_mutex_lock(&db_mtx);
	write_log(_log,2,"Execuete query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL||(db_row=mysql_fetch_row(db_res))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"603;authority not found");
		return;
	}
	int authority=atoi(db_row[0]);
	if(0==authority&READABLE){	//NOT READABLE
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"604;cannnot readable");
		return;
	}
	
	sprintf(query,"SELECT date,link FROM diary WHERE id='%d'",diary_id);
	write_log(_log,2,"Execuete query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL||(db_row=mysql_fetch_row(db_res))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"605;diary not found");
		return;
	}
	char version[256]="",loc[256]="";
	strcpy(version,db_row[0]);
	strcpy(loc,db_row[1]);

	pthread_mutex_unlock(&db_mtx);	

	//location build need
	char buff[BUFF_SIZE]="";
	FILE* fp=fopen(loc,"rb");
	if(fp==NULL){
		message_to_client(_log,cli,"606;cannnot open file");
		return;
	}
	fseek(fp,0,SEEK_END);
	int f_size=ftell(fp);
	fseek(fp,0,SEEK_SET);
	int len=sprintf(buff,"600;%d;%s;%d",diary_id,version,f_size);
	message_to_client(_log,cli,buff,len);
	len=message_from_client(_log,cli,buff);
	for(int i=0;i<f_size;i+=len){
		len=fread(buff,1,BUFF_SIZE,fp);
		send(cli->s,buff,len,0);
	}
	fclose(fp);
}
void usersearch(FILE* _log,struct client_info *cli,char* next_ptr){
	if(false/*!*cli->id*/){//have not session
		message_to_client(_log,cli,"701;not signed");
		return;
	}
	char* user_id=strtok_r(NULL,";",&next_ptr);
	if(user_id==NULL||strtok_r(NULL,";",&next_ptr)!=NULL){
		message_to_client(_log,cli,"702;download sentence error");
		return;
	}
	char query[BUFF_SIZE]="";
	sprintf(query,"SELECT nickname FROM user WHERE id = '%s'",user_id);
	pthread_mutex_lock(&db_mtx);
	write_log(_log,2,"Execuete query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL||(db_row=mysql_fetch_row(db_res))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"703;user not found");
		return;
	}
	char nickname[50]="";
	strcpy(nickname,db_row[0]);
	pthread_mutex_unlock(&db_mtx);	
	char buff[BUFF_SIZE]="";
	int len=sprintf(buff,"700;%s;%s",user_id,nickname);
	message_to_client(_log,cli,buff,len);
}
void befriend(FILE* _log,struct client_info *cli,char* next_ptr){
	if(!*cli->id){//have not session
		message_to_client(_log,cli,"801;not signed");
		return;
	}
	char* tgt_id=strtok_r(NULL,";",&next_ptr);
	if(tgt_id==NULL||strtok_r(NULL,";",&next_ptr)!=NULL){
		message_to_client(_log,cli,"802;befriend sentence error");
		return;
	}
	char query[BUFF_SIZE]="";
	sprintf(query,"SELECT accepted FROM friend WHERE to_user_id='%s' AND from_user_id='%s'",cli->id,tgt_id);	//select friend request
	pthread_mutex_lock(&db_mtx);
	write_log(_log,2,"Execute query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"803;friend select error");
		return;
	}
	int is_accept=0;
	if((db_row=mysql_fetch_row(db_res))==NULL){	//request be freind
		sprintf(query,"INSERT INTO friend(from_user_id, to_user_id, accepted) VALUES('%s','%s','0')",cli->id,tgt_id);
		write_log(_log,2,"Execute query : ",query);
		if(mysql_noreturn_query(db_conn,query)){
			pthread_mutex_unlock(&db_mtx);
			message_to_client(_log,cli,"804;friend insert error");
			return;
		}
	}
	else if(is_accept=!strcmp(db_row[0],"0")){	//accept friend request
		sprintf(query,"UPDATE friend SET accepted=1 WHERE to_user_id='%s' AND from_user_id='%s'",cli->id,tgt_id);
		write_log(_log,2,"Execute query : ",query);
		if(mysql_noreturn_query(db_conn,query)){
			pthread_mutex_unlock(&db_mtx);
			message_to_client(_log,cli,"806;friend accept failed");
			return;
		}
		sprintf(query,"INSERT INTO friend(from_user_id, to_user_id, accepted) VALUES('%s','%s','1')",cli->id,tgt_id);
		write_log(_log,2,"Execute query : ",query);
		if(mysql_noreturn_query(db_conn,query)){
			pthread_mutex_unlock(&db_mtx);
			message_to_client(_log,cli,"805;friend request failed");
			return;
		}
	}
	else{
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"807;already friend");
		return;
	}
	pthread_mutex_unlock(&db_mtx);
	message_to_client(_log,cli,"800;befriend success");
}
void friendlist(FILE* _log,struct client_info *cli,char* next_ptr){
	if(!*cli->id){//have not session
		message_to_client(_log,cli,"901;not signed");
		return;
	}
	const int page_size=10;
	char* page_s=strtok_r(NULL,";",&next_ptr);
	int page=atoi(page_s)*page_size;
	if(page_s==NULL||strtok_r(NULL,";",&next_ptr)!=NULL){
		message_to_client(_log,cli,"902;friendlist sentence error");
		return;
	}
	char query[BUFF_SIZE]="",buff[BUFF_SIZE]="";
	sprintf(query,"SELECT to_user_id FROM friend WHERE from_user_id='%s' AND accepted='1' LIMIT %d,%d",cli->id,page,page_size);	//select friend request
	pthread_mutex_lock(&db_mtx);
	write_log(_log,2,"Execute query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"903;friend select error");
		return;
	}
	const char*header="900;{";
	char*p=buff+sprintf(buff,header);
	while((db_row=mysql_fetch_row(db_res))==NULL){
		p=p+sprintf(p,"%s,",db_row[0]);
	}
	pthread_mutex_unlock(&db_mtx);
	if(p==buff+strlen(header))p++;
	sprintf(p-1,"}");
	message_to_client(_log,cli,buff,p-buff);
}
void wantbefriend(FILE* _log,struct client_info *cli,char* next_ptr){	
	if(!*cli->id){//have not session
		message_to_client(_log,cli,"1001;not signed");
		return;
	}
	const int page_size=10;
	char* page_s=strtok_r(NULL,";",&next_ptr);
	int page=atoi(page_s)*page_size;
	if(page_s==NULL||strtok_r(NULL,";",&next_ptr)!=NULL){
		message_to_client(_log,cli,"1002;wantbefriend sentence error");
		return;
	}
	char query[BUFF_SIZE]="",buff[BUFF_SIZE]="";
	sprintf(query,"SELECT to_user_id FROM friend WHERE from_user_id='%s' AND accepted='0' LIMIT %d,%d",cli->id,page,page_size);	//select friend request
	pthread_mutex_lock(&db_mtx);
	write_log(_log,2,"Execute query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"1003;want friend select error");
		return;
	}
	const char*header="1000;{";
	char*p=buff+sprintf(buff,header);
	while((db_row=mysql_fetch_row(db_res))==NULL){
		p=p+sprintf(p,"%s,",db_row[0]);
	}
	pthread_mutex_unlock(&db_mtx);
	if(p==buff+strlen(header))p++;
	sprintf(p-1,"}");
	message_to_client(_log,cli,buff,p-buff);
}
void toshare(FILE* _log,struct client_info *cli,char* next_ptr){	
	if(!*cli->id){//have not session
		message_to_client(_log,cli,"1101;not signed");
		return;
	}
	char* diary_id_str=strtok_r(NULL,";",&next_ptr);
	char* tgt_id=strtok_r(NULL,";",&next_ptr);
	char* authority_str=strtok_r(NULL,";",&next_ptr);
	int diary_id=atoi(diary_id_str);
	int authority=atoi(authority_str);
	if(diary_id_str==NULL||tgt_id==NULL||authority_str==NULL||diary_id==0||authority==0||strtok_r(NULL,";",&next_ptr)!=NULL){	
		message_to_client(_log,cli,"1102;toshare sentence error");
		return;
	}
	if(!strcmp(tgt_id,cli->id)){
		message_to_client(_log,cli,"1103;cannot change authority about you");
		return;
	}
	char query[BUFF_SIZE]="",buff[BUFF_SIZE]="";
	sprintf(query,"SELECT * FROM authority WHERE user_id='%s' AND diary_id='%d' AND authority='65535'",cli->id,diary_id);
	pthread_mutex_lock(&db_mtx);
	write_log(_log,2,"Execute query : ",query);
	if((db_res=mysql_return_query(db_conn,query))==NULL||(db_row=mysql_fetch_row(db_res))==NULL){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"1104;diary not found or you are not master");
		return;
	}
	sprintf(query,"INSERT INTO authority(user_id,diary_id,authority) VALUES('%s','%d','%d') ON DUPLICATE KEY UPDATE authority='%d'",cli->id,diary_id,authority,authority);
	write_log(_log,2,"Execute query : ",query);
	if(mysql_noreturn_query(db_conn,query)){
		pthread_mutex_unlock(&db_mtx);
		message_to_client(_log,cli,"1105;authority upsert error");
		return;
	}
	message_to_client(_log,cli,"1100;authority set success");
	pthread_mutex_unlock(&db_mtx);
}
