#include "confutil.h"
#include "mysqlutil.h"

#define MILLION (1000 * 1000)

struct order {
  unsigned long id;
  int is_buy;
  char issued[64];
  int duration;
  int reg_id;
  unsigned long loc_id;
  int sys_id;
  unsigned int type_id;
  double price;
  unsigned long vol_rem;
  unsigned long min_vol;
};

struct item {
  unsigned int id;
  int group_id;
  int mgroup_id;
  float volume;
  char name[256];
};

char *build_query(int is_buy, unsigned int type_id, int min_order_amount,
                  char *price_ord, int o_qty);
int get_orders(struct order orders[], int o_qty, MYSQL *mh, int is_buy,
               unsigned int type_id, unsigned long min_ord_amount,
               char *price_ord);
char *get_region_name(MYSQL *mh, int id);
char *get_system_name(MYSQL *mh, int id);
int check_deals(MYSQL *mh, config conf, struct item *);
int print_report(MYSQL *mh, config conf);
