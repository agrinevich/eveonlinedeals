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

struct sde_region {
  unsigned int id;
  char *name;
};

struct sde_constellation {
  unsigned int id;
  unsigned int reg_id;
  char *name;
};

struct sde_solarsystem {
  unsigned int id;
  unsigned int con_id;
  char *name;
};

char *build_path(char *base_path, char *sub_path);
int regions2db(char *dir, MYSQL *con);
int constellations2db(char *dir, MYSQL *con);
int ss2db(char *dir, MYSQL *con);
int types2db(char *dir, MYSQL *con);
char *strdup(const char *s);
int sync_static_data(MYSQL *con);
