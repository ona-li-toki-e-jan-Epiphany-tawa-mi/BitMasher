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

BitMasher, a text adventure game where you act as an antivirus attempting to rid a computer of a RANSOMWARE attack.

## Controls

Each option you can select will be shown by a single character in paranthesis. Typing that character and hitting ENTER will select that option.

## How to Play

You are an antivirus trying to rid a computer of a RANSOMWARE before it takes over the system. There is a finite amount of time before the system is fully infected.

In order to defeat it, you must find all items before you find the RANSOMWARE. If you do not, you will not be able to EXTRACT it and you will lose.

Each system (room) contains an item, which you can move to; UP, DOWN, LEFT, AND RIGHT. Keep in mind that the map is NOT 2D; Moving RIGHT, UP, LEFT, and DOWN will lead to a different room than the one you started in. The map is "Spiky" so-to-speak.

You have a SCANner to aid in figuring out which rooms contain items and which have RANSOMWARE. Using the SCANner will reveal what the surronding rooms contain, and the room you are currently in will be automatically SCANned for you. But beware: SCANning takes time. Also, occasionaly a SCAN will fail and need to be repeated.

## Configuration

There is a section at the top of the program file ([TextBasedGame.py](src/bitmasher_game/TextBasedGame.py "BitMasher program file")) containing configuration options. All config options bear a description detailing what they modify and what values they accept.

## Installation

BitMasher is avalible on PyPi, and can be installed by running the following command(s):

```console
pip install bitmasher_game
```

Alternatively, you can download the latest package from [Releases,](https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/BitMasher/releases "BitMasher releases.") or build from source, and install it using the following command(s):

```console
pip install <package name goes here>
```

BitMasher can also be ran directly as a simple script without installation. See [How to Run](README.md#how-to-run "How to run section.")

## How to run

If installed, run the following command(s):

```console
bitmasher
```

BitMasher is, itself, a stand-alone script ([TextBasedGame.py](src/bitmasher_game/TextBasedGame.py "TextBasedGame.py")), and can be ran without installation by running one of the following command(s) in the project directory:

```console
python3 src/bitmasher_game/TextBasedGame.py
./src/bitmasher_game/TextBasedGame.py
```

## How to build

Run the following commands in the project directory:

```console
python3 -m build
```

The built packages will appear in [dist/.](dist "Distributables folder.")

## Dependencies

The only dependency is [Python 3.](https://www.python.org "Python homepage")

## Links

Demonstration:<br>
https://www.youtube.com/watch?v=zxC5pLrn6N8