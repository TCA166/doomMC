
all: mcTypes.o networkingMc.o src/server.cpp cNBT.o
	g++ $(CFLAGS) -o server src/server.cpp mcTypes.o networkingMc.o cNBT.o -lpthread -lz

mcTypes.o: src/mcTypes.c
	gcc $(CFLAGS) -c src/mcTypes.c

networkingMc.o: src/networkingMc.c
	gcc $(CFLAGS) -c src/networkingMc.c

cNBT.o: cNBT/buffer.c cNBT/nbt_parsing.c cNBT/nbt_treeops.c cNBT/nbt_util.c
	gcc cNBT/buffer.c -o cNBT/buffer.o -c $(CFLAGS)
	gcc cNBT/nbt_parsing.c -o cNBT/nbt_parsing.o -c $(CFLAGS)
	gcc cNBT/nbt_treeops.c -o cNBT/nbt_treeops.o -c $(CFLAGS)
	gcc cNBT/nbt_util.c -o cNBT/nbt_util.o -c $(CFLAGS)
	ld -relocatable cNBT/buffer.o cNBT/nbt_parsing.o cNBT/nbt_treeops.o cNBT/nbt_util.o -o cNBT.o
