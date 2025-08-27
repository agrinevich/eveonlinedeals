#include "sync_sd.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <yaml.h>

char *build_path(char *base_path, char *sub_path) {

  size_t nbytes = snprintf(NULL, 0, "%s/%s", base_path, sub_path) + 1;
  char *full_path = malloc(nbytes);
  snprintf(full_path, nbytes, "%s/%s", base_path, sub_path);

  // free full_path !
  return full_path;
}

int is_name_dot(char *name) {
  char dot1[] = ".";
  int result1 = strncmp(name, dot1, 1);
  if (result1 == 0) {
    return 1;
  }

  char dot2[] = "..";
  int result2 = strncmp(name, dot2, 2);
  if (result2 == 0) {
    return 1;
  }

  return 0;
}

int sys2db(unsigned int con_id, char *sys_dir, char *sys_name, MYSQL *con) {
  // read solarsystem.yaml
  yaml_parser_t parser;
  if (!yaml_parser_initialize(&parser)) {
    fprintf(stderr, "Failed to initialize parser!\n");
    return 1;
  }

  char *yaml_file = build_path(sys_dir, "solarsystem.yaml");
  FILE *fp = fopen(yaml_file, "rb");
  yaml_parser_set_input_file(&parser, fp);

  // from whole YAML file we want sys_id only
  yaml_event_t event;
  char *scalar_value = "";
  unsigned int id = 0;
  int id_found = 0;
  int done = 0;

  while (!done) {
    if (!yaml_parser_parse(&parser, &event)) {
      fprintf(stderr, "Parser error: %s\n", parser.problem);
      yaml_parser_delete(&parser);
      fclose(fp);
      return 1;
    }

    if (event.type == YAML_SCALAR_EVENT) {
      scalar_value = (char *)event.data.scalar.value;

      if (id_found == 1) {
        id = atol(scalar_value);
      }
      if (strcmp(scalar_value, "solarSystemID") == 0) {
        id_found = 1;
      } else {
        id_found = 0;
      }
    }

    if (event.type == YAML_STREAM_END_EVENT) {
      done = 1;
    }

    yaml_event_delete(&event);
  }

  fclose(fp);
  free(yaml_file);
  yaml_parser_delete(&parser);

  // save solarsystem info to DB
  // printf("\t\t%s: %u\n", sys_name, id);
  char ins[256];
  char *ins_fmt =
      "INSERT INTO solar_system (id, con_id, name) VALUES (%u, %u, '%s')";
  sprintf(ins, ins_fmt, id, con_id, sys_name);
  if (mysql_query(con, ins)) {
    finish_with_error(con);
  }

  return 0;
}

int con2db(unsigned int reg_id, char *con_dir, char *con_name, MYSQL *con) {
  // read constellation.yaml
  yaml_parser_t parser;
  if (!yaml_parser_initialize(&parser)) {
    fprintf(stderr, "Failed to initialize parser!\n");
    return 1;
  }

  char *yaml_file = build_path(con_dir, "constellation.yaml");
  FILE *fp = fopen(yaml_file, "rb");
  yaml_parser_set_input_file(&parser, fp);

  // from whole YAML file we want con_id only
  yaml_event_t event;
  char *scalar_value = "";
  unsigned int id = 0;
  int id_found = 0;
  int done = 0;

  while (!done) {
    if (!yaml_parser_parse(&parser, &event)) {
      fprintf(stderr, "Parser error: %s\n", parser.problem);
      yaml_parser_delete(&parser);
      fclose(fp);
      return 1;
    }

    if (event.type == YAML_SCALAR_EVENT) {
      scalar_value = (char *)event.data.scalar.value;

      if (id_found == 1) {
        id = atol(scalar_value);
      }
      if (strcmp(scalar_value, "constellationID") == 0) {
        id_found = 1;
      } else {
        id_found = 0;
      }
    }

    if (event.type == YAML_STREAM_END_EVENT) {
      done = 1;
    }

    yaml_event_delete(&event);
  }

  fclose(fp);
  free(yaml_file);
  yaml_parser_delete(&parser);

  // save constellation info to DB
  // printf("\t%s: %u\n", con_name, id);
  char ins[256];
  char *ins_fmt =
      "INSERT INTO constellation (id, reg_id, name) VALUES (%u, %u, '%s')";
  sprintf(ins, ins_fmt, id, reg_id, con_name);
  if (mysql_query(con, ins)) {
    finish_with_error(con);
  }

  // now we want solar systems list
  DIR *dp = opendir(con_dir);
  if (dp == NULL) {
    perror("Failed to opendir");
    return 1;
  }

  struct dirent *entry;
  struct stat filestat;
  while ((entry = readdir(dp))) {
    if (is_name_dot(entry->d_name)) {
      continue;
    }

    char *sys_dir = build_path(con_dir, entry->d_name);

    stat(sys_dir, &filestat);

    if (S_ISDIR(filestat.st_mode)) {
      sys2db(id, sys_dir, entry->d_name, con);
    }

    free(sys_dir);
  }

  closedir(dp);

  return 0;
}

int region2db(char *reg_dir, char *reg_name, MYSQL *con) {
  // read region.yaml

  yaml_parser_t parser;
  if (!yaml_parser_initialize(&parser)) {
    fprintf(stderr, "Failed to initialize parser!\n");
    return 1;
  }

  char *yaml_file = build_path(reg_dir, "region.yaml");
  FILE *fp = fopen(yaml_file, "rb");
  yaml_parser_set_input_file(&parser, fp);

  yaml_event_t event;
  char *scalar_value = "";
  unsigned int region_id = 0;
  int region_id_found = 0;
  int done = 0;

  while (!done) {
    if (!yaml_parser_parse(&parser, &event)) {
      fprintf(stderr, "Parser error: %s\n", parser.problem);
      yaml_parser_delete(&parser);
      fclose(fp);
      return 1;
    }

    if (event.type == YAML_SCALAR_EVENT) {
      scalar_value = (char *)event.data.scalar.value;

      if (region_id_found == 1) {
        region_id = atol(scalar_value);
      }
      if (strcmp(scalar_value, "regionID") == 0) {
        region_id_found = 1;
      } else {
        region_id_found = 0;
      }
    }

    if (event.type == YAML_STREAM_END_EVENT) {
      done = 1;
    }

    yaml_event_delete(&event);
  }

  fclose(fp);
  free(yaml_file);
  yaml_parser_delete(&parser);

  // save region_id, reg_name to DB
  printf("%s: %u\n", reg_name, region_id);
  char ins[256];
  char *ins_fmt = "INSERT INTO region (id, name) VALUES (%u, '%s')";
  sprintf(ins, ins_fmt, region_id, reg_name);
  if (mysql_query(con, ins)) {
    finish_with_error(con);
  }

  DIR *dp = opendir(reg_dir);
  if (dp == NULL) {
    perror("Failed to opendir");
    return 1;
  }

  struct dirent *entry;
  struct stat filestat;
  while ((entry = readdir(dp))) {
    if (is_name_dot(entry->d_name)) {
      continue;
    }

    char *con_dir = build_path(reg_dir, entry->d_name);

    stat(con_dir, &filestat);

    if (S_ISDIR(filestat.st_mode)) {
      con2db(region_id, con_dir, entry->d_name, con);
    }

    free(con_dir);
  }

  closedir(dp);

  return 0;
}

int universe2db(char *dir, MYSQL *con) {

  DIR *dp = opendir(dir);
  if (dp == NULL) {
    perror("Failed to read dir");
    return 1;
  }

  if (mysql_query(con, "TRUNCATE TABLE region")) {
    finish_with_error(con);
  }

  if (mysql_query(con, "TRUNCATE TABLE constellation")) {
    finish_with_error(con);
  }

  if (mysql_query(con, "TRUNCATE TABLE solar_system")) {
    finish_with_error(con);
  }

  struct dirent *entry;
  struct stat filestat;
  while ((entry = readdir(dp))) {
    if (is_name_dot(entry->d_name)) {
      continue;
    }

    char *reg_dir = build_path(dir, entry->d_name);

    stat(reg_dir, &filestat);

    if (S_ISDIR(filestat.st_mode)) {
      region2db(reg_dir, entry->d_name, con);
    }

    free(reg_dir);
  }

  closedir(dp);

  return 0;
}

int types2db(char *dir, MYSQL *con) {

  if (mysql_query(con, "TRUNCATE TABLE types")) {
    finish_with_error(con);
  }

  yaml_parser_t parser;
  if (!yaml_parser_initialize(&parser)) {
    fprintf(stderr, "Failed to initialize parser!\n");
    return 1;
  }

  char *yaml_file = build_path(dir, "types.yaml");
  FILE *fp = fopen(yaml_file, "rb");
  yaml_parser_set_input_file(&parser, fp);

  yaml_event_t event;
  struct sde_type item;
  char *scalar_value = "";
  unsigned int id = 0;
  int is_bprice = 0;
  int is_group_id = 0;
  int is_mgroup_id = 0;
  int is_name = 0;
  int is_name_en = 0;
  int is_pub = 0;
  int is_volume = 0;
  int level = 0;
  int done = 0;
  // int limit = 1000;
  int i = 0;

  while (!done) {
    if (!yaml_parser_parse(&parser, &event)) {
      fprintf(stderr, "Parser error: %s\n", parser.problem);
      yaml_parser_delete(&parser);
      fclose(fp);
      return 1;
    }

    if (event.type == YAML_STREAM_START_EVENT) {
      printf("Stream start\n");
    }

    if (event.type == YAML_MAPPING_START_EVENT) {
      if (level == 1) {
        i++;
      }
      level++;
    }

    if (event.type == YAML_SCALAR_EVENT) {
      scalar_value = (char *)event.data.scalar.value;

      if (level == 1) {
        id = atol(scalar_value);
        item.id = id;
      } else if (level == 2) {
        if (strcmp(scalar_value, "basePrice") == 0) {
          is_bprice = 1;
        } else if (is_bprice == 1) {
          item.bprice = atof(scalar_value);
          is_bprice = 0;
        } else if (strcmp(scalar_value, "groupID") == 0) {
          is_group_id = 1;
        } else if (is_group_id == 1) {
          item.group_id = atol(scalar_value);
          is_group_id = 0;
        } else if (strcmp(scalar_value, "marketGroupID") == 0) {
          is_mgroup_id = 1;
        } else if (is_mgroup_id == 1) {
          item.mgroup_id = atol(scalar_value);
          is_mgroup_id = 0;
        } else if (strcmp(scalar_value, "published") == 0) {
          is_pub = 1;
        } else if (is_pub == 1) {
          if (strcmp(scalar_value, "true") == 0) {
            item.pub = 1;
          } else {
            item.pub = 0;
          }
          is_pub = 0;
        } else if (strcmp(scalar_value, "volume") == 0) {
          is_volume = 1;
        } else if (is_volume == 1) {
          item.volume = atof(scalar_value);
          is_volume = 0;
        } else if (strcmp(scalar_value, "name") == 0) {
          is_name = 1;
        }
      } else if (level == 3 && is_name == 1) {
        if (strcmp(scalar_value, "en") == 0) {
          is_name_en = 1;
        } else if (is_name_en == 1) {
          item.name = strdup(scalar_value);
          if (item.name == NULL) {
            perror("Abort: memory allocation by strdup failed\n");
            return 1;
          }
          is_name_en = 0;
        }
      }
    }

    if (event.type == YAML_MAPPING_END_EVENT) {
      if (level == 2) {
        // save item to DB
        // printf("id: %u\n", item.id);
        // printf("group_id: %u\n", item.group_id);
        // printf("mgroup_id: %u\n", item.mgroup_id);
        // printf("volume: %lf\n", item.volume);
        // printf("name: %s\n\n", item.name);
        char ins[512];
        char escaped_name[256];
        mysql_real_escape_string(con, escaped_name, item.name,
                                 strlen(item.name));
        char *ins_fmt =
            "INSERT INTO types (id, pub, group_id, mgroup_id, bprice, volume, "
            "name) VALUES (%u, %d, %u, %u, \"%lf\", \"%lf\", \"%s\")";
        sprintf(ins, ins_fmt, item.id, item.pub, item.group_id, item.mgroup_id,
                item.bprice, item.volume, escaped_name);
        if (mysql_query(con, ins)) {
          finish_with_error(con);
        }

        // reset structure and free memory ?
        free(item.name);
        item = (struct sde_type){0};
        is_name = 0;
      }

      level--;
    }

    if (event.type == YAML_STREAM_END_EVENT) {
      done = 1;
      printf("Stream end\n");
    }

    // if (i == limit && level == 1) {
    //   done = 1;
    // }

    yaml_event_delete(&event);
  }

  free(yaml_file);
  fclose(fp);

  yaml_parser_delete(&parser);

  return 0;
}

int sync_static_data(MYSQL *con) {

  char *universe_dir = "./sde/universe/eve";
  universe2db(universe_dir, con);

  char *fsd_dir = "./sde/fsd";
  types2db(fsd_dir, con);

  return 0;
}
