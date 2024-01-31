
CFLAGS += -Wunused-result -Wall -Wunused-parameter

all: server

debug: CFLAGS+=-Werror -g
debug: all

fast: CFLAGS+=-Ofast 
fast: all

server: src/main.cpp complete.o
	g++ $(CFLAGS) -o server src/main.cpp complete.o -lspdlog -lfmt -lpthread -lz

complete.o: mcTypes.o networkingMc.o server.o cNBT.o lobby.o player.o cJSON.o client.o maps.o regionParser.o entity.o
	ld -relocatable server.o mcTypes.o networkingMc.o cNBT.o lobby.o player.o client.o maps.o cJSON.o regionParser.o entity.o -o complete.o

server.o: src/server.cpp
	g++ $(CFLAGS) -c src/server.cpp

lobby.o: src/lobby.cpp
	g++ $(CFLAGS) -c src/lobby.cpp

player.o: src/player.cpp
	g++ $(CFLAGS) -c src/player.cpp

client.o: src/client.cpp
	g++ $(CFLAGS) -c src/client.cpp

maps.o : src/map/map.cpp src/map/udmf.cpp src/map/mcr.cpp
	g++ $(CFLAGS) -c src/map/map.cpp
	g++ $(CFLAGS) -c src/map/udmf.cpp
	g++ $(CFLAGS) -c src/map/mcr.cpp
	ld -relocatable map.o udmf.o mcr.o -o maps.o

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

regionParser.o: src/C/regionParser.c
	gcc src/C/regionParser.c -o regionParser.o -c $(CFLAGS)

entity.o: src/entity.cpp
	g++ $(CFLAGS) -c src/entity.cpp

clean:
	rm -f *.o
	rm -f cNBT/*.o
	rm -f cJSON/*.o

check: debug
	checkmk tests/C/mcTypesTests.check > tests/C/cTestsRunner.c
	gcc tests/C/cTestsRunner.c mcTypes.o cNBT.o -lcheck -lm -Wall -lz -lsubunit -lrt -lpthread -o tests/C/cTestsRunner
	./tests/C/cTestsRunner

tests/fuzz/serverFuzzer.o: tests/fuzz/serverFuzzer.cpp
	g++ tests/fuzz/serverFuzzer.cpp -c -o tests/fuzz/serverFuzzer.o -g

basicFuzzer: debug tests/fuzz/serverFuzzer.o tests/fuzz/basicFuzzer.cpp
	g++ tests/fuzz/basicFuzzer.cpp complete.o tests/fuzz/serverFuzzer.o -o tests/fuzz/basicFuzzer -lspdlog -lfmt -lpthread -lz -g
	gdb -ex=r --args tests/fuzz/basicFuzzer 2

packetFuzzer: debug tests/fuzz/serverFuzzer.o tests/fuzz/packetFuzzer.cpp
	g++ tests/fuzz/packetFuzzer.cpp complete.o tests/fuzz/serverFuzzer.o -o tests/fuzz/packetFuzzer -lspdlog -lfmt -lpthread -lz -g
	gdb -ex=r --args tests/fuzz/packetFuzzer 2

requirements:
	sudo apt install libspdlog-dev libfmt-dev
	sudo apt-get install check
	sudo apt install zlib1g zlib1g-dev
