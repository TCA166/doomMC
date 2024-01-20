# doomMC

Linux Minecraft server meant to host a DOOM style TDM game.

## Overview

The general idea behind this project is to use the Minecraft internet protocol to create a unique player experience.
There is nothing in the protocol itself that requires the game sync'd with the protocol to be an open world survival sandbox.
So if we were to say convert DOOM maps to blocks, send these blocks once and make the map immutable, and hijack packets for clicking left click as shooting packets we would have a basic DOOM TDM server.

## Lobbies

Once the player starts connecting they are put into a random lobby.
Each lobby is running a single map.
Additionally each lobby has two threads, one for receiving player data, and the other one for performing game ticks.
Player synchronization overall is split "equally" between the two threads.

## Maps

The server by default needs maps placed in the /maps subdirectory next to the server executable.
As of right now the server supports two types of map files:

- [mcr](https://minecraft.fandom.com/wiki/Region_file_format) files - minecraft region files
- [udmf](https://doomwiki.org/wiki/UDMF) files - doom maps stored in the udmf format

## Install

In order to run this server you will need to compile it.
This software is exclusively for Linux and trying to compile it under other *nix systems will result in problems.

1. Install make
2. Install requirements
    If you are running a Debian derived Linux run the requirements target.

    ```bash
    make requirements
    ```

    Otherwise find the needed packages in your manager of choice based on the target specification.
3. Compile
    Run the server target and it should work as is and compile fine assuming you have all the necessary requirements installed.

    ```bash
    make server
    ```

## Static files

In order for the server to properly run some static files need to be available next to it.
These are:

- registryCodec.nbt - this is a raw nbt file containing the registry coded. Extracted from a vanilla 1.19.4 server via a [modified client](https://github.com/TCA166/segfaultCraft)
- tags.bin - raw contents of the [Update Tags](./https://wiki.vg/index.php?title=Protocol&oldid=18242#Update_Tags) packet extracted from a vanilla 1.19.4 server
- status.json - json template that will be sent to be displayed on the server list
- version.json - Json definition of all blocks and items in Minecraft 1.19.4 taken from [PixLyzer data](https://gitlab.bixilon.de/bixilon/pixlyzer-data)([mirror](https://github.com/bixilon/pixlyzer-data))

## Development

As of right now the project is still under development.
The server works in the very basic sense.
I am still struggling with player synchronization, a big milestone will be having two players in a lobby being able to see each other and having the position and rotation accurately.
Feel free to help.

### Big TODO

Long term TODO

1. Support for older than 1.19.4
2. Support for newer than 1.19.4
3. Tests
4. Fuzzing (libfuzzer)
5. Position verification
6. Anticheat
7. Plugins for commands
