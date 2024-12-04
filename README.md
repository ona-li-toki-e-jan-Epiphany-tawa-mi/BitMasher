```text
 ______  __________________ _______  _______  _______           _______  _______
(  ___ \ \__   __/\__   __/(       )(  ___  )(  ____ \|\     /|(  ____ \(  ____ )
| (   ) )   ) (      ) (   | () () || (   ) || (    \/| )   ( || (    \/| (    )|
| (__/ /    | |      | |   | || || || (___) || (_____ | (___) || (__    | (____)|
|  __ (     | |      | |   | |(_)| ||  ___  |(_____  )|  ___  ||  __)   |     __)
| (  \ \    | |      | |   | |   | || (   ) |      ) || (   ) || (      | (\ (
| )___) )___) (___   | |   | )   ( || )   ( |/\____) || )   ( || (____/\| ) \ \__
|/ \___/ \_______/   )_(   |/     \||/     \|\_______)|/     \|(_______/|/   \__/
```

# BitMasher

A text adventure game where you act as an antivirus attempting to rid a computer
of a RANSOMWARE attack.

## How to build

Dependencies:

- A C compiler supporting c11. Clang or GCC recommended.
- POSIX system.

Then, run the following command(s):

```sh
./build.sh
```

To enable optimizations, you can add one or more of the following arguments to
the EXTRA_CFLAGS enviroment variable:

- `-O3` - general optimizations.
- `-DNDEBUG -Wno-return-type` - disable safety checks. Performance > safety.

I.e.:

```sh
EXTRA_CFLAGS='-O3 -DNDEBUG -Wno-return-type' ./build.sh
```

The executable will be named `bitmasher`.

## Configuration

There is a section at the top of the program file `bitmasher.c` containing
configuration options. All config options bear a description detailing what they
modify and what values they accept.

## Installation

You can install BitMasher with Nix from the NUR
([https://github.com/nix-community/NUR](https://github.com/nix-community/NUR))
with the following attribute:

```nix
nur.repos.ona-li-toki-e-jan-Epiphany-tawa-mi.bitmasher
```

## Controls

Each option you can select will be shown by a single character in paranthesis.
Typing that character and hitting ENTER will select that option.

## How to play

You are an antivirus trying to rid a computer of a RANSOMWARE before it takes
over the system. There is a finite amount of time before the system is fully
infected.

In order to defeat it, you must find all items before you find the RANSOMWARE.
If you do not, you will not be able to EXTRACT it and you will lose.

Each system (room) contains an item, which you can move to; UP, DOWN, LEFT, AND
RIGHT. Keep in mind that the map is NOT 2D; Moving RIGHT, UP, LEFT, and DOWN
will lead to a different room than the one you started in. The map is "Spiky"
so-to-speak.

You have a SCANner to aid in figuring out which rooms contain items and which
have RANSOMWARE. Using the SCANner will reveal what the surronding rooms
contain, and the room you are currently in will be automatically SCANned for
you. But beware: SCANning takes time. Also, occasionaly a SCAN will fail and
need to be repeated.

## Release notes

- Rewrote in C for funsies.
- Now restores terminal state on shutdown.
