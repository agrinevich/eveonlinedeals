#include <mysql/mysql.h>
#include <stdio.h>

// TODO rename to "mysql_bailout"
void finish_with_error(MYSQL *con) {
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}
