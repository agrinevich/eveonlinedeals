#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "confutil.h"
#include "mysqlutil.h"
#include "report.h"
#include "sync_o.h"
#include "sync_sd.h"

static int config_handler(void *conf, const char *section, const char *name,
                          const char *value) {
  // config instance for filling in the values.
  config *pconfig = (config *)conf;

// define a macro for checking Sections and keys under the sections.
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

  // fill the values in config struct for Section 1.
  if (MATCH("mysql", "user")) {
    pconfig->db.user = strdup(value);
  } else if (MATCH("mysql", "pass")) {
    pconfig->db.pass = strdup(value);
  } else if (MATCH("mysql", "dbname")) {
    pconfig->db.dbname = strdup(value);
  } else if (MATCH("report", "min_amount")) {
    pconfig->rep.min_amount = atoi(value) * MILLION;
  } else if (MATCH("report", "sql_types")) {
    pconfig->rep.sql_types = strdup(value);
  } else {
    return 0;
  }

  return 1;
}

// TODO add time logging

int main(int argc, char **argv) {
  // read args

  int dflag = 0; // sync data from local files
  int oflag = 0; // sync orders from ESI server
  int rflag = 0; // analyze and report best deals
  // char *cvalue = NULL;
  int index;
  int c;

  // while ((c = getopt(argc, argv, "dorc:")) != -1) {
  while ((c = getopt(argc, argv, "dor")) != -1) {
    switch (c) {
    case 'd':
      dflag = 1;
      break;
    case 'o':
      oflag = 1;
      break;
    case 'r':
      rflag = 1;
      break;
    // case 'c':
    //   cvalue = optarg;
    //   break;
    case '?':
      if (optopt == 'c')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint(optopt))
        fprintf(stderr, "Unknown option `-%c'.\n", optopt);
      else
        fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
      return 1;
    default:
      abort();
    }
  }

  for (index = optind; index < argc; index++) {
    printf("Non-option argument %s\n", argv[index]);
  }

  // read config.inni

  config conf;

  if (ini_parse("config.ini", config_handler, &conf) < 0) {
    printf("Failed to read config file\n");
    return 1;
  }

  print_config(conf);

  // connect to mysql

  MYSQL *mh = mysql_init(NULL);
  if (mh == NULL) {
    fprintf(stderr, "mysql_init() failed\n");
    exit(1);
  }

  if (mysql_real_connect(mh, "localhost", conf.db.user, conf.db.pass,
                         conf.db.dbname, 0, NULL, 0) == NULL) {
    finish_with_error(mh);
  }

  // run command

  if (dflag == 1) {
    sync_static_data(mh);
  } else if (oflag == 1) {
    sync_orders(mh);
  } else if (rflag == 1) {
    print_report(mh, conf);
  } else {
    printf("Nothing to do\n");
  }

  // free memory

  free((void *)conf.db.user);
  free((void *)conf.db.pass);
  free((void *)conf.db.dbname);
  free((void *)conf.rep.sql_types);
  mysql_close(mh);

  return 0;
}
