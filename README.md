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

## How to Build

Dependencies:

- A C compiler supporting C99.
- POSIX or DOS system.

### POSIX

Clang, GCC, or Zig recommended.

Run the following command(s):

```sh
./build.sh -std=c99
```

Compiler defaults to cc. Set the CC environment variable to override it.

To enable optimizations, you can append on or more of the following arguments to
the build command:

- `-O3` - general optimizations.
- `-DNDEBUG` - disable safety checks. Performance > safety.

I.e.:

```sh
./build.sh -std=c99 -O3 -DNDEBUG
```

The executable will be named `bitmasher`.

### DOS (cross-compilation)

DJGPP recommended - [https://www.delorie.com/djgpp/](https://www.delorie.com/djgpp/)

Run `build.sh` after setting the CC environment variable to the cross
compiler. I.e.:

```sh
CC=i586-pc-msdosdjgpp-gcc ./build.sh
```

To enable optimizations, you can append on or more of the following arguments to
the build command:

- `-O3` - general optimizations.
- `-DNDEBUG` - disable safety checks. Performance > safety.

I.e.:

```sh
CC=i586-pc-msdosdjgpp-gcc ./build.sh -O3 -DNDEBUG
```

The executable will be named `bitmasher.exe`.

### DOS

DJGPP recommended - [https://www.delorie.com/djgpp/](https://www.delorie.com/djgpp/)

Just compile `bitmasher.c` directly with whatever C compiler you happen to
have. Make sure to add the `include` directory.

## Configuration

There is a section at the top of the program file `bitmasher.c` containing
configuration options. All config options bear a description detailing what they
modify and what values they accept.

## Installation

You can install it with Nix from my personal package repository
[https://paltepuk.xyz/cgit/epitaphpkgs.git/about](https://paltepuk.xyz/cgit/epitaphpkgs.git/about).

## Controls

Each option you can select will be shown by a single character in paranthesis.
Typing that character and hitting ENTER will select that option.

## How to Play

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
