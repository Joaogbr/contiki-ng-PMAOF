CONTIKI_PROJECT = udp-client rsu normal-vehicle broken-vehicle
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = sky native


MODULES_REL += ./messages

NEEDS_CONTIKI_VERSION_FILES += messages.c

CONTIKI=../..

MAKE_MAC = MAKE_MAC_CSMA
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

#USE_LIBM ?= 1

# Energy usage estimation
MODULES += os/services/simple-energest

include $(CONTIKI)/Makefile.dir-variables
include $(CONTIKI)/Makefile.include
