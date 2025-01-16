#ifndef __MYSQL_CONT_H__
#define __MYSQL_CONT_H__
struct connection_details{
	char *server, *user, *password, *database;
};
MYSQL* mysql_connection_setup(struct connection_details mysql_details);
MYSQL_RES* mysql_return_query(MYSQL* connection, const char* sql_query);
int mysql_noreturn_query(MYSQL* connection,const char* sql_query);
#endif//__MYSQL_CONT_H__
