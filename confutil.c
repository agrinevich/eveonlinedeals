#include "confutil.h"

void print_config(config conf) {
  printf("\nCONFIG\n-----\n\n");

  printf("db\n");
  printf("user: %s\n", conf.db.user);
  printf("pass: %s\n", conf.db.pass);
  printf("dbname: %s\n", conf.db.dbname);

  printf("report\n");
  printf("min_amount: %lu M\n", conf.rep.min_amount / (1000 * 1000));
  printf("sql_types: %s\n", conf.rep.sql_types);

  printf("\n");
}
