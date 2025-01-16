#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(){
	DIR* dp;
	FILE* fp;
	struct dirent *entry;
	struct stat stat_buf;
	char buff[1024]="",*sp,*np,*bp=buff+strlen(buff);
	if((fp=fopen("link.sh","wt"))==NULL){
		printf("link.sh open Error");
		return 1;
	}
	if((dp=opendir("./"))==NULL){
		printf("Directory open Error");
		return 1;
	}
	while((entry=readdir(dp))!=NULL){
		lstat(entry->d_name,&stat_buf);
		if(!S_ISDIR(stat_buf.st_mode)){
			for(sp=entry->d_name;(np=strchr(sp+1,'.'))!=NULL;sp=np);
			if(!strcmp(sp,".cpp")){
				fprintf(fp,"gcc -c %s -I/usr/include/mysql\n",entry->d_name);
				fprintf(fp,"echo gcc -c %s complete\n",entry->d_name);
				sp[1]='o';
				sp[2]=0;
				bp+=sprintf(bp," %s",entry->d_name);
			}
		}
	}
	fclose(fp);
	if((fp=fopen("build.sh","wt"))==NULL){
		printf("build.sh open Error");
		return 1;
	}
	fprintf(fp,"gcc -o server -lpthread -L/usr/lib64/mysql -lmysqlclient%s\n",buff);
	fprintf(fp,"echo gcc -o server%s complete\n",buff);
	fclose(fp);
	if(chmod("link.sh",0755)<0){
		printf("chmod 755 link.sh failed");
		return 1;
	}
	if(chmod("build.sh",0755)<0){
		printf("chmod 755 build.sh failed");
		return 1;
	}
	return 0;
}
