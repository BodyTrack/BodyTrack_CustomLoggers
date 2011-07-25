all: basestation cheststrap

bs basestation:
	make -f Makefile.basestation

bsp basestation-program:
	make -f Makefile.basestation program

cs cheststrap:
	make -f Makefile.cheststrap

csp cheststrap-program:
	make -f Makefile.cheststrap program

