CONTIKI_PROJECT = zabbix-agent
all: zabbix-proto-agent
#CONTIKI_WITH_IPV4 = 1
#CONTIKI_WITH_IPV6 = 1
WITH_UIP6=1
UIP_CONF_IPV6=1
CONTIKI = $(PATHCONTIKI)
include $(CONTIKI)/Makefile.include

ifeq ($(PREFIX),)
 PREFIX = aaaa::1/64
endif

$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)

connect-router:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 $(PREFIX)

connect-router-cooja:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 $(PREFIX)
#	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 -p 10050 $(PREFIX)
