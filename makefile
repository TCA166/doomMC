
all: server

server: mcTypes.o networkingMc.o src/server.cpp cNBT.o lobby.o player.o cJSON.o client.o maps.o
	g++ $(CFLAGS) -o server src/server.cpp mcTypes.o networkingMc.o cNBT.o lobby.o player.o client.o maps.o cJSON.o -lspdlog -lfmt -lpthread -lz

lobby.o: src/lobby.cpp
	g++ $(CFLAGS) -c src/lobby.cpp

player.o: src/player.cpp
	g++ $(CFLAGS) -c src/player.cpp

client.o: src/client.cpp
	g++ $(CFLAGS) -c src/client.cpp

maps.o : src/map/map.cpp src/map/udmf.cpp src/map/minecraftMap.cpp
	g++ $(CFLAGS) -c src/map/map.cpp
	g++ $(CFLAGS) -c src/map/udmf.cpp
	g++ $(CFLAGS) -c src/map/minecraftMap.cpp
	ld -relocatable map.o udmf.o minecraftMap.o -o maps.o

mcTypes.o: src/C/mcTypes.c
	gcc $(CFLAGS) -c src/C/mcTypes.c

networkingMc.o: src/C/networkingMc.c
	gcc $(CFLAGS) -c src/C/networkingMc.c

cNBT.o: cNBT/buffer.c cNBT/nbt_parsing.c cNBT/nbt_treeops.c cNBT/nbt_util.c cNBT/nbt_loading.c
	gcc cNBT/nbt_loading.c -o cNBT/nbt_loading.o -c $(CFLAGS)
	gcc cNBT/buffer.c -o cNBT/buffer.o -c $(CFLAGS)
	gcc cNBT/nbt_parsing.c -o cNBT/nbt_parsing.o -c $(CFLAGS)
	gcc cNBT/nbt_treeops.c -o cNBT/nbt_treeops.o -c $(CFLAGS)
	gcc cNBT/nbt_util.c -o cNBT/nbt_util.o -c $(CFLAGS)
	ld -relocatable cNBT/nbt_loading.o cNBT/buffer.o cNBT/nbt_parsing.o cNBT/nbt_treeops.o cNBT/nbt_util.o -o cNBT.o

cJSON.o: cJSON/cJSON.c
	gcc cJSON/cJSON.c -o cJSON.o -c $(CFLAGS)

clean:
	rm -f *.o
	rm -f cNBT/*.o
	rm -f cJSON/*.o
	rm -f server
