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

View build status/get prebuilt wheel packages:
- I2P: http://oytjumugnwsf4g72vemtamo72vfvgmp4lfsf6wmggcvba3qmcsta.b32.i2p/hydra/jobset/bitmasher/master/latest-eval
- Tor: http://4blcq4arxhbkc77tfrtmy4pptf55gjbhlj32rbfyskl672v2plsmjcyd.onion/hydra/jobset/bitmasher/master/latest-eval

# BitMasher

A text adventure game where you act as an antivirus attempting to rid a computer
of a RANSOMWARE attack.

## How to run

Dendencies:

- Python 3

Then, run one of the following commands to get started:

```
python3 src/bitmasher_game/TextBasedGame.py
```

## Installation

BitMasher is avalible on PyPi (https://pypi.org/project/bitmasher-game), and can
be installed by running the following command(s):

```console
pip install bitmasher_game
```

You can also install it with Nix from the NUR
(https://github.com/nix-community/NUR) with the following attribute:

```nix
nur.repos.ona-li-toki-e-jan-Epiphany-tawa-mi.bitmasher
```

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

## How to Build (for distribution)

Dependencies:
- Python 3.
- Python `build` module.

There is a `flake.nix` you can use with `nix develop path:.` to generate a
development environment.

Then, run the following command(s) in the project directory:

```console
python3 -m build
```

The built packages will appear in `dist/`.

## Links

Demonstration:<br>
https://odysee.com/@ona-li-toki-e-jan-Epiphany-tawa-mi:9/BitMasher-A-fast-paced-text-based-game-born-from-my-frustration-and-pettiness:d?r=HYroMZaqrVN4gL5oSJ35gcTgt3K56r39

## Release Notes

- "Improved" lose sequence.
