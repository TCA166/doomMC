# doomMC

## Static files

In order for the server to properly run some static files need to be available next to it.
These are:

- registryCodec.nbt - this is a raw nbt file containing the registry coded. Extracted from a vanilla 1.19.4 server via a [modified client](https://github.com/TCA166/segfaultCraft)
- tags.bin - raw contents of the [Update Tags](./https://wiki.vg/index.php?title=Protocol&oldid=18242#Update_Tags) packet extracted from a vanilla 1.19.4 server
- status.json - json template that will be sent to be displayed on the server list
- version.json - Json definition of all blocks and items in Minecraft 1.19.4 taken from [PixLyzer data](https://gitlab.bixilon.de/bixilon/pixlyzer-data)([mirror](https://github.com/bixilon/pixlyzer-data))

## Big TODO

1. Support for older than 1.19.4
2. Support for newer than 1.19.4
3. Tests
4. Fuzzing (libfuzzer)
5. Position verification
6. Anticheat
7. Plugins for commands
