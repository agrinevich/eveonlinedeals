DEBUG = 1
EXECUTABLE_NAME = evedeals

curl_LIBS := $(shell curl-config --libs)
curl_FLAGS := $(shell curl-config --cflags)

mysql_LIBS := $(shell mysql_config --libs)
mysql_FLAGS := $(shell mysql_config --cflags)

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99
CFLAGS2 = -Wall -Wextra -Wpedantic -std=c89
CPPFLAGS = $(curl_FLAGS) $(mysql_FLAGS)
LDFLAGS = ${curl_LIBS} $(mysql_LIBS)

ifeq ($(DEBUG), 1)
CFLAGS += -g -O0
CFLAGS2 += -g -O0
else
CFLAGS += -O3
CFLAGS2 += -O3
endif

COMPILER_CALL = $(CC) $(CFLAGS) ${CPPFLAGS}
COMPILER_CALL2 = $(CC) $(CFLAGS2)

build: cJSON.o ini.o confutil.o mysqlutil.o sync_sd.o sync_o.o report.o evedeals.o
	$(COMPILER_CALL) evedeals.o confutil.o mysqlutil.o sync_sd.o sync_o.o report.o cJSON.o ini.o $(LDFLAGS) -o $(EXECUTABLE_NAME) -lm -lyaml

evedeals.o:
	$(COMPILER_CALL) evedeals.c -c

confutil.o:
	$(COMPILER_CALL) confutil.c -c

mysqlutil.o:
	$(COMPILER_CALL) mysqlutil.c -c

sync_sd.o:
	$(COMPILER_CALL) sync_sd.c -c

sync_o.o:
	$(COMPILER_CALL) sync_o.c -c
	
report.o:
	$(COMPILER_CALL) report.c -c
	
cJSON.o:
	$(COMPILER_CALL2) cJSON.c -c

ini.o:
	$(COMPILER_CALL) ini.c -c

clean:
	rm -f *.o
	rm -f $(EXECUTABLE_NAME)
