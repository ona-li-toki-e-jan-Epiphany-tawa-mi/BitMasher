# BitMasher

BitMasher, a text adventure game where you act as an antivirus attempting to rid a computer of a RANSOMWARE attack.

## Controls

Each option you can select will be shown by a single character in paranthesis. Typing that character and hitting ENTER will select that option.

## How to Play

You are an antivirus trying to rid a computer of a RANSOMWARE before it takes over the system. There is a finite amount of time before the system is fully infected.

In order to defeat it, you must find all items before you find the RANSOMWARE. If you do not, you will not be able to EXTRACT it and you will lose.

Each system (room) contains an item, which you can move to; UP, DOWN, LEFT, AND RIGHT. Keep in mind that the map is NOT 2D; Moving RIGHT, UP, LEFT, and DOWN will lead to a different room than the one you started in. The map is "Spiky" so-to-speak.

You have a SCANner to aid in figuring out which rooms contain items and which have RANSOMWARE. Using the SCANner will reveal what the surronding rooms contain, and the room you are currently in will be automatically SCANned for you. But beware: SCANning takes time. Also, occasionaly a SCAN will fail and need to be repeated.

## How to Run

The only requirement is having a Python intepreter installed on your system.

Execute the following command(s) in the project directory:

```console
$ python3 TextBasedGame.py
```