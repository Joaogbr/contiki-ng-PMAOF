CONTIKI_PROJECT = udp-client udp-client-cam udp-client-sleep udp-server
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = sky native

CONTIKI=../..

MAKE_MAC = MAKE_MAC_CSMA
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

#USE_LIBM ?= 1

# Energy usage estimation
MODULES += os/services/simple-energest

include $(CONTIKI)/Makefile.dir-variables
include $(CONTIKI)/Makefile.include
