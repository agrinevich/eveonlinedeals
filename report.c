#include "report.h"
#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>

// TODO move "min_price" to config
char *build_query(int is_buy, unsigned int type_id, int min_amount,
                  char *price_ord, int o_qty) {
  char *tpl = "SELECT\
                id,\
                issued,\
                duration,\
                reg_id,\
                loc_id,\
                sys_id,\
                price,\
                vol_rem\
              FROM orders\
              WHERE \
                is_buy = %d AND\
                type_id = %d AND\
                price > 500 AND\
                price * vol_rem > %d\
              ORDER BY \
                price %s, vol_rem DESC\
              LIMIT %d";
  size_t nbytes =
      snprintf(NULL, 0, tpl, is_buy, type_id, min_amount, price_ord, o_qty) + 1;
  char *query = malloc(nbytes);
  snprintf(query, nbytes, tpl, is_buy, type_id, min_amount, price_ord, o_qty);

  // printf("%s\n", query);
  // free after use!
  return query;
}

int get_orders(struct order a_orders[], int o_qty, MYSQL *mh, int is_buy,
               unsigned int type_id, unsigned long min_amount,
               char *price_ord) {

  char *q = build_query(is_buy, type_id, min_amount, price_ord, o_qty);
  if (mysql_query(mh, q)) {
    free(q);
    finish_with_error(mh);
  }
  free(q);

  MYSQL_RES *morders = mysql_store_result(mh);
  if (morders == NULL) {
    // TODO print query before exit
    finish_with_error(mh);
  }

  unsigned long orders_found = mysql_num_rows(morders);

  MYSQL_ROW order_row;
  int i = 0;
  while ((order_row = mysql_fetch_row(morders))) {
    a_orders[i].id = atol(order_row[0]);
    strcpy(a_orders[i].issued, order_row[1]);
    a_orders[i].duration = atoi(order_row[2]);
    a_orders[i].reg_id = atoi(order_row[3]);
    a_orders[i].loc_id = atol(order_row[4]);
    a_orders[i].sys_id = atoi(order_row[5]);
    a_orders[i].price = atof(order_row[6]);
    a_orders[i].vol_rem = atol(order_row[7]);

    i++;
  }

  mysql_free_result(morders);
  return orders_found;
}

// something wrong with returning strings ?
char *get_region_name(MYSQL *mh, int id) {
  char *tpl = "SELECT name FROM region WHERE id = %d";
  size_t nbytes = snprintf(NULL, 0, tpl, id) + 1;
  char *query = malloc(nbytes);
  snprintf(query, nbytes, tpl, id);

  if (mysql_query(mh, query)) {
    free(query);
    finish_with_error(mh);
  }
  free(query);

  MYSQL_RES *result = mysql_store_result(mh);
  if (result == NULL) {
    finish_with_error(mh);
  }

  MYSQL_ROW row = mysql_fetch_row(result);
  char *reg_name = row[0];

  mysql_free_result(result);

  return reg_name;
}

// something wrong with returning strings ?
char *get_system_name(MYSQL *mh, int id) {
  char *tpl = "SELECT name FROM solar_system WHERE id = %d";
  size_t nbytes = snprintf(NULL, 0, tpl, id) + 1;
  char *query = malloc(nbytes);
  snprintf(query, nbytes, tpl, id);

  if (mysql_query(mh, query)) {
    free(query);
    finish_with_error(mh);
  }
  free(query);

  MYSQL_RES *result = mysql_store_result(mh);
  if (result == NULL) {
    finish_with_error(mh);
  }

  MYSQL_ROW row = mysql_fetch_row(result);
  char *sys_name = row[0];

  mysql_free_result(result);

  return sys_name;
}

int check_deals(MYSQL *mh, config conf, struct item *it) {
  // sell orders

  int is_buy = 0;
  int sell_o_limit = 5;
  char *price_ord = "ASC";
  struct order a_sell_orders[sell_o_limit];
  int found_sell = get_orders(a_sell_orders, sell_o_limit, mh, is_buy, it->id,
                              conf.rep.min_amount, price_ord);
  if (found_sell == 0) {
    // printf("\n%s: no sell orders\n", it->name);
    return 0;
  }

  // printf("\nSell orders: %d\n", found_sell);
  // for (int i = 0; i < found_sell; i++) {
  //   printf("%lu\t%lf\t%lu\n", a_sell_orders[i].id, a_sell_orders[i].price,
  //          a_sell_orders[i].vol_rem);
  // }

  // buy orders

  is_buy = 1;
  price_ord = "DESC";
  int buy_o_limit = 5;
  struct order a_buy_orders[buy_o_limit];
  int found_buy = get_orders(a_buy_orders, buy_o_limit, mh, is_buy, it->id,
                             conf.rep.min_amount, price_ord);
  if (found_buy == 0) {
    // printf("\n%s: no buy orders\n", it->name);
    return 0;
  }

  // printf("\nBuy orders: %d\n", found_buy);
  // for (int i = 0; i < found_buy; i++) {
  //   printf("%lu\t%lf\t%lu\n", a_buy_orders[i].id, a_buy_orders[i].price,
  //          a_buy_orders[i].vol_rem);
  // }

  // check if it's profitable
  if (a_sell_orders[0].price >= a_buy_orders[0].price) {
    // printf("\n%s: no profit\n", it->name);
    return 0;
  }

  printf("\n%s\n", it->name);

  // start and destination

  char *reg_from = get_region_name(mh, a_sell_orders[0].reg_id);
  printf("%s ", reg_from);
  char *sys_from = get_system_name(mh, a_sell_orders[0].sys_id);
  printf("/ %s ---> ", sys_from);

  char *reg_to = get_region_name(mh, a_buy_orders[0].reg_id);
  printf("%s ", reg_to);
  char *sys_to = get_system_name(mh, a_buy_orders[0].sys_id);
  printf("/ %s\n", sys_to);

  // quantity to buy/sell

  unsigned long qty = 0;
  if (a_sell_orders[0].vol_rem < a_buy_orders[0].vol_rem) {
    qty = a_sell_orders[0].vol_rem;
  } else {
    qty = a_buy_orders[0].vol_rem;
  }

  // cargo volume

  float vol = it->volume * qty;
  printf("Volume: %lu * %f =  %f\n", qty, it->volume, vol);

  // possible profit for best sell and buy order

  double amount2buy = a_sell_orders[0].price * qty;
  double amount2sell = a_buy_orders[0].price * qty;
  double profit = amount2sell - amount2buy;
  printf("Profit: %lf M\n", profit / MILLION);

  return 0;
}

int print_report(MYSQL *mh, config conf) {

  char *sel2 = conf.rep.sql_types;
  if (mysql_query(mh, sel2)) {
    finish_with_error(mh);
  }

  MYSQL_RES *types = mysql_store_result(mh);
  if (types == NULL) {
    // TODO print query before exit
    finish_with_error(mh);
  }

  MYSQL_ROW type_row;
  struct item it = {0};
  // int types_qty = mysql_num_rows(types);
  // unsigned long a_types[types_qty];
  // int j = 0;
  while ((type_row = mysql_fetch_row(types))) {
    it.id = atol(type_row[0]);
    it.volume = atof(type_row[1]);
    strcpy(it.name, type_row[2]);

    check_deals(mh, conf, &it);
    // a_types[j] = atoi(type_row[0]);
    // j++;
  }

  mysql_free_result(types);

  // int num = sizeof(a_types) / sizeof(a_types[0]);
  // for (int i = 0; i < num; i++) {
  //   check_deals(mh, a_types[i]);
  // }

  return 0;
}
