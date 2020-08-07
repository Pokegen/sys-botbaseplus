.PHONY: all clean

all:
	@$(MAKE) -C sys-botbaseplus/

clean:
	@$(MAKE) clean -C sys-botbaseplus/
