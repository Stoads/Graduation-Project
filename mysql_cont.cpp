#include <mysql.h>
#include <cstdlib>
#include <cstdio>

#include "handling.h"
#include "mysql_cont.h"

MYSQL* mysql_connection_setup(struct connection_details mysql_details){
        MYSQL* connection = mysql_init(NULL);
        if(!mysql_real_connect(connection, mysql_details.server, mysql_details.user, mysql_details.password, mysql_details.database,0,NULL,0))return NULL;
        return connection;
}
MYSQL_RES* mysql_return_query(MYSQL* connection, const char* sql_query){
        if(mysql_query(connection, sql_query))return NULL;
        return mysql_store_result(connection);
}
int mysql_noreturn_query(MYSQL* connection, const char* sql_query){
        if(mysql_query(connection, sql_query))return 1;
	return 0;
}
