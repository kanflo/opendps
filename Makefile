ifneq ($(V), 1)
MFLAGS += --no-print-dir
Q := @
endif

all:
	$(Q)if [ ! -f libopencm3/Makefile ]; then \
		echo "Initialising git submodules..." ;\
		git submodule init ;\
		git submodule update ;\
	fi
	$(Q)$(MAKE) $(MFLAGS) -C libopencm3 TARGETS=stm32/f1
	$(Q)$(MAKE) $(MFLAGS) -C opendps
	$(Q)$(MAKE) $(MFLAGS) -C dpsboot
	$(Q)$(MAKE) $(MFLAGS) -C esp8266-proxy

clean:
	$(Q)$(MAKE) $(MFLAGS) -C libopencm3 $@
	$(Q)$(MAKE) $(MFLAGS) -C opendps $@
	$(Q)$(MAKE) $(MFLAGS) -C dpsboot $@
	$(Q)$(MAKE) $(MFLAGS) -C esp8266-proxy $@

flash:
	$(Q)$(MAKE) $(MFLAGS) -C opendps $@
	$(Q)$(MAKE) $(MFLAGS) -C dpsboot $@
