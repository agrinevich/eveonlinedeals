#include "mysqlutil.h"
#include <curl/curl.h>

struct region {
  int id;
  char name[256];
};

int replace_char(char *str, char old_char, char new_char);
char *slurp_file(const char *filename);
char *build_url(int rid, int page);
char *build_filename(int rid, int page);
int url2file(CURL *ch, const char *url, const char *file);
int fetch_orders(MYSQL *con, CURL *ch, int reg_id);
int sync_orders(MYSQL *con);
