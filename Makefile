.PHONY: all clean

all:
	@$(MAKE) -C Atmosphere-libs/
	@$(MAKE) -C sys-botbaseplus/

clean:
	@$(MAKE) clean -C Atmosphere-libs/
	@$(MAKE) clean -C sys-botbaseplus/
