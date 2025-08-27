#ifndef CONFUTIL_H
#define CONFUTIL_H

#include "ini.h"

typedef struct {
  const char *user;
  const char *pass;
  const char *dbname;
} db_params;

typedef struct {
  unsigned long min_amount;
  char *sql_types;
} report_params;

typedef struct {
  db_params db;
  report_params rep;
} config;

void print_config(config conf);
// static int config_handler(void *conf, const char *section, const char *name,
//                           const char *value);

#endif // CONFUTIL_H
