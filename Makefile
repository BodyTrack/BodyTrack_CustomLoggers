all: basestation cheststrap basestationbootloader

bs basestation:
	make -f Makefile.basestation

bsp basestation-program:
	make -f Makefile.basestation program

cs cheststrap:
	make -f Makefile.cheststrap

csp cheststrap-program:
	make -f Makefile.cheststrap program
	
bsbl basestationbootloader:
	make -f Makefile.basestationbootloader

bsblp basestationbootloader-program:
	make -f Makefile.basestationbootloader program
	
	

