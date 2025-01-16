gcc -o server -lpthread -L/usr/lib64/mysql -lmysqlclient handling.o mysql_cont.o child_proc.o main.o
echo gcc -o server handling.o mysql_cont.o child_proc.o main.o complete
