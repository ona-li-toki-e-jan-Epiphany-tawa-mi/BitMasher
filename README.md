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

## Configuration

There is a section at the top of the program file
`src/bitmasher_game/TextBasedGame.py` containing configuration options. All
config options bear a description detailing what they modify and what values
they accept.

## How to run

You will need Python 3 installed on your system.

BitMasher is, itself, a stand-alone Python script
`src/bitmasher_game/TextBasedGame.py`, and can be ran without installation.

If installed, run the following command(s):

```console
bitmasher
```

## Installation

BitMasher is avalible on [(CLEARNET) PyPi,](https://pypi.org/project/bitmasher-game "Bitmasher page on PyPi")
and can be installed by running the following command(s):

```console
pip install bitmasher_game
```

You can also install it with Nix from the [(CLEARNET) NUR](https://github.com/nix-community/NUR)
with the following attribute:

```nix
nur.repos.ona-li-toki-e-jan-Epiphany-tawa-mi.bitmasher
```

## How to build

You will need Python 3 and the Python build module installed on your system.
There is a `shell.nix` you can use with `nix-shell` to get them.

Run the following commands in the project directory:

```console
python3 -m build
```

The built packages will appear in `dist/`.

## Release Notes

- "Improved" lose sequence.

## Bugs

Report bugs to:<br>
https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/BitMasher/issues

## Links

Demonstration:<br>
https://odysee.com/@ona-li-toki-e-jan-Epiphany-tawa-mi:9/BitMasher-A-fast-paced-text-based-game-born-from-my-frustration-and-pettiness:d?r=HYroMZaqrVN4gL5oSJ35gcTgt3K56r39
