#include "cJSON.h"
#include "sync_o.h"
#include <curl/curl.h>
#include <curl/header.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int replace_char(char *str, char old_char, char new_char) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == old_char) {
      str[i] = new_char;
    }
  }
  return 0;
}

char *slurp_file(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("Error opening file");
    return NULL;
  }

  // Determine file size
  fseek(fp, 0, SEEK_END);
  size_t file_size = ftell(fp);
  rewind(fp);

  // Allocate memory
  char *buffer = (char *)malloc(file_size + 1); // +1 for null terminator
  if (buffer == NULL) {
    perror("Error allocating memory");
    fclose(fp);
    return NULL;
  }

  // Read file content
  size_t bytes_read = fread(buffer, 1, file_size, fp);
  if (bytes_read != file_size) {
    perror("Error reading file");
    free(buffer);
    fclose(fp);
    return NULL;
  }

  // Null-terminate the buffer
  buffer[file_size] = '\0';

  // don't forget to free buffer !
  fclose(fp);
  return buffer;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

int url2file(CURL *ch, const char *url, const char *file) {
  // FILE *fh;
  // CURLcode res;
  // struct curl_header *h_pages;
  // struct curl_header *h_err_remain;
  // struct curl_header *h_err_reset;

  curl_easy_setopt(ch, CURLOPT_URL, url);

  FILE *fh = fopen(file, "wb");
  if (fh == NULL) {
    fprintf(stderr, "Failed to open file: %s\n", file);
    exit(1);
  }

  curl_easy_setopt(ch, CURLOPT_WRITEDATA, fh);

  CURLcode res = curl_easy_perform(ch);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
    fclose(fh);
    exit(1);
  }
  fclose(fh);

  struct curl_header *h_pages;
  if (curl_easy_header(ch, "X-Pages", 0, CURLH_HEADER, -1, &h_pages) ==
      CURLHE_OK) {
    // printf("Pages: %s\n", h_pages->value);
    return atoi(h_pages->value);
  }

  // if (curl_easy_header(ch, "X-ESI-Error-Limit-Remain", 0, CURLH_HEADER, -1,
  //                      &h_err_remain) == CURLHE_OK) {
  //   printf("Errors remain: %s\n", h_err_remain->value);
  // }

  // if (curl_easy_header(ch, "X-ESI-Error-Limit-Reset", 0, CURLH_HEADER, -1,
  //                      &h_err_reset) == CURLHE_OK) {
  //   printf("Errors reset: %s\n", h_err_reset->value);
  // }

  return 1;
}

char *build_url(int rid, int page) {

  // char *tpl =
  //     "https://esi.evetech.net/markets/%d/orders?order_type=all&type_id=%d";
  char *tpl =
      "https://esi.evetech.net/markets/%d/orders?order_type=all&page=%d";
  size_t nbytes = snprintf(NULL, 0, tpl, rid, page) + 1;
  char *url = malloc(nbytes);
  snprintf(url, nbytes, tpl, rid, page);

  // free after use!
  return url;
}

char *build_filename(int rid, int page) {

  char *tpl = "./orders/%d-%d.json";
  size_t nbytes = snprintf(NULL, 0, tpl, rid, page) + 1;
  char *buff = malloc(nbytes);
  snprintf(buff, nbytes, tpl, rid, page);

  // free after use!
  return buff;
}

int save_orders(MYSQL *mh, int rid, char *file) {
  char *json_string = slurp_file(file);

  cJSON *json_arr = cJSON_Parse(json_string);
  if (cJSON_GetErrorPtr()) {
    printf("Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
    return -1;
  }

  int orders_qty = cJSON_GetArraySize(json_arr);
  // printf("Orders: %d\n", orders_qty);

  cJSON *obj;
  cJSON *order_id;
  cJSON *duration;
  cJSON *is_buy_order;
  cJSON *issued;
  cJSON *loc_id;
  cJSON *min_vol;
  cJSON *price;
  cJSON *sys_id;
  cJSON *type_id;
  cJSON *vol_remain;

  char *id_fmt;
  char *duration_fmt;
  char *is_buy_order_fmt;
  int is_buy;
  char *issued_fmt;
  char *loc_id_fmt;
  char *min_vol_fmt;
  char *price_fmt;
  char *sys_id_fmt;
  char *type_id_fmt;
  char *vol_remain_fmt;

  char *instpl = "REPLACE INTO orders (id, is_buy, issued, duration, reg_id, "
                 "loc_id, sys_id, type_id, price, vol_rem, min_vol) VALUES "
                 "(%s, %d, '%s', %s, %d, %s, %s, %s, '%s', %s, %s)";
  char *ins;
  size_t nbytes;

  for (int i = 0; i < orders_qty; i++) {

    obj = cJSON_GetArrayItem(json_arr, i);

    order_id = cJSON_GetObjectItem(obj, "order_id");
    id_fmt = cJSON_Print(order_id);

    duration = cJSON_GetObjectItem(obj, "duration");
    duration_fmt = cJSON_Print(duration);

    is_buy_order = cJSON_GetObjectItem(obj, "is_buy_order");
    is_buy_order_fmt = cJSON_Print(is_buy_order);
    if (strcmp(is_buy_order_fmt, "true") == 0) {
      is_buy = 1;
    } else {
      is_buy = 0;
    }

    issued = cJSON_GetObjectItem(obj, "issued");
    issued_fmt = cJSON_Print(issued);
    replace_char(issued_fmt, '"', ' ');
    replace_char(issued_fmt, 'T', ' ');
    replace_char(issued_fmt, 'Z', '\0');

    loc_id = cJSON_GetObjectItem(obj, "location_id");
    loc_id_fmt = cJSON_Print(loc_id);

    min_vol = cJSON_GetObjectItem(obj, "min_volume");
    min_vol_fmt = cJSON_Print(min_vol);

    price = cJSON_GetObjectItem(obj, "price");
    price_fmt = cJSON_Print(price);

    sys_id = cJSON_GetObjectItem(obj, "system_id");
    sys_id_fmt = cJSON_Print(sys_id);

    type_id = cJSON_GetObjectItem(obj, "type_id");
    type_id_fmt = cJSON_Print(type_id);

    vol_remain = cJSON_GetObjectItem(obj, "volume_remain");
    vol_remain_fmt = cJSON_Print(vol_remain);

    nbytes = snprintf(NULL, 0, instpl, id_fmt, is_buy_order_fmt, issued_fmt,
                      duration_fmt, rid, loc_id_fmt, sys_id_fmt, type_id,
                      price_fmt, vol_remain_fmt, min_vol_fmt) +
             1;
    ins = malloc(nbytes);
    snprintf(ins, nbytes, instpl, id_fmt, is_buy, issued_fmt, duration_fmt, rid,
             loc_id_fmt, sys_id_fmt, type_id_fmt, price_fmt, vol_remain_fmt,
             min_vol_fmt);
    if (mysql_query(mh, ins)) {
      printf("ins=%s\n", ins);
      free(ins);
      finish_with_error(mh);
    }
    free(ins);
  }

  cJSON_Delete(json_arr);

  return orders_qty;
}

int fetch_orders(MYSQL *mh, CURL *ch, int rid) {
  char *file = build_filename(rid, 1);
  char *url = build_url(rid, 1);
  int page_qty = url2file(ch, url, file);
  save_orders(mh, rid, file);
  free(url);
  free(file);

  if (page_qty > 1) {
    for (int p = 2; p <= page_qty; p++) {
      file = build_filename(rid, p);
      url = build_url(rid, p);
      url2file(ch, url, file);
      save_orders(mh, rid, file);
      free(url);
      free(file);
    }
  }

  return page_qty;
}

int sync_orders(MYSQL *con) {

  CURL *curlh;

  curl_global_init(CURL_GLOBAL_ALL);
  curlh = curl_easy_init();

  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, "X-Compatibility-Date: 2020-01-01");
  chunk = curl_slist_append(chunk, "User-Agent: EveDeals (grinevich@gmail.com; "
                                   "eve:a153) libcurl/8.12.1");

  curl_easy_setopt(curlh, CURLOPT_HTTPHEADER, chunk);
  // curl_easy_setopt(curlh, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curlh, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curlh, CURLOPT_WRITEFUNCTION, write_data);

  if (mysql_query(con, "TRUNCATE TABLE orders")) {
    finish_with_error(con);
  }

  // fetch regions to array

  char *sel = "SELECT id, name FROM region ORDER BY name ASC";
  if (mysql_query(con, sel)) {
    finish_with_error(con);
  }

  MYSQL_RES *regions = mysql_store_result(con);
  if (regions == NULL) {
    finish_with_error(con);
  }

  MYSQL_ROW region_row;
  int region_qty = mysql_num_rows(regions);
  struct region a_regions[region_qty];
  int i = 0;
  while ((region_row = mysql_fetch_row(regions))) {
    strcpy(a_regions[i].name, region_row[1]);
    a_regions[i].id = atoi(region_row[0]);
    i++;
  }

  mysql_free_result(regions);

  // fetch all orders for each region

  int page_qty = 0;
  for (i = 0; i < region_qty; i++) {
    page_qty = fetch_orders(con, curlh, a_regions[i].id);
    printf("%d\t%s\n", page_qty, a_regions[i].name);

    sleep(1);
  }

  curl_easy_cleanup(curlh);
  curl_slist_free_all(chunk);
  curl_global_cleanup();

  return 0;
}
