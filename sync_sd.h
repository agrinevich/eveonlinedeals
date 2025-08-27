#include "mysqlutil.h"

struct sde_type {
  unsigned int id;
  double bprice;
  unsigned int group_id;
  unsigned int mgroup_id;
  char *name;
  int pub;
  double volume;
};

char *build_path(char *base_path, char *sub_path);
int is_name_dot(char *name);
int sys2db(unsigned int id, char *sys_dir, char *sys_name, MYSQL *con);
int con2db(unsigned int id, char *con_dir, char *con_name, MYSQL *con);
int region2db(char *reg_dir, char *reg_name, MYSQL *con);
int universe2db(char *dir, MYSQL *con);
int types2db(char *dir, MYSQL *con);
char *strdup(const char *s);
int sync_static_data(MYSQL *con);
