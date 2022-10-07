#!/usr/bin/env python3

"""
BitMasher, a text adventure game where you act as an antiviris attempting to rid a computer of a 
    ransomware attack.

@author Nathaniel Needham
"""

import enum
import os
import shutil
import sys
import time



def centerMessage(message: str) -> str:
    """ Returns message with the spacing required to appear centered in the terminal. """
    return message.center(shutil.get_terminal_size().columns)

def delayedPrint(message='', end='\n', center=False):
    """ Used to give a slow-scroll effect akin to old computers like the Commodore 64 and Apple II. """
    time.sleep(0.15)
    print(message if not center else centerMessage(message), end=end)

def clearScreen():
    """ Clears the terminal. """
    os.system('cls' if os.name=='nt' else 'clear')
    # NOTE: Consider adding in the following line. Will need to check if terminal supports ANSI escape
    #   codes.
    # print("\033[H\033[2J", end="")


class OptionSelector:
    """ Used to easily create selection menus where users are given a choice among a list of options.
        
        Use addOption() to set up the menu, and then use getSelection() to have the user select 
        something. """

    def __init__(self):
        self.options = []
        self.messages = []

    #TODO: add input checking on character code.
    def addOption(self, characterCode: str, message: str):
        """ Adds a new option. The character code is what the user will type to select that option. 
        Put only a single character, will be made lowercase if possible. """
        self.options.append(characterCode.lower())
        self.messages.append(message)

    def getSelection(self) -> str:
        """ Prompts the user with the options and has them make a selection. """
        for message in self.messages:
            delayedPrint(message)

        while True:
            choice = input().lower()

            if len(choice) == 0 or choice[0] not in self.options:
                delayedPrint(f"Error: Invalid option '{choice}'!")
                continue

            return choice


def exitGame():
    """ Displays an exiting message and then exits. """
    delayedPrint("Exiting", end='')
    delayedPrint(".", end='')
    delayedPrint(".", end='')
    delayedPrint(".")

    sys.exit(0)


class Direction(enum.Enum):
    """ Represents a physical direction in which to travel. """
    UP = enum.auto()
    DOWN = enum.auto()
    LEFT = enum.auto()
    RIGHT = enum.auto()

    #TODO: make this use a dict.
    def opposite(self) -> Direction:
        """ Returns the direction opposite to the current one. """
        if self is Direction.UP:
            return Direction.DOWN
        elif self is Direction.DOWN:
            return Direction.UP
        elif self is Direction.LEFT:
            return Direction.RIGHT
        else:
            return Direction.LEFT
        
class Room:
    """ Represents a room within the game. """
    def __init__(self):
        self.adjacentRooms = { Direction.UP:    None,
                               Direction.DOWN:  None,
                               Direction.LEFT:  None,
                               Direction.RIGHT: None  }
        self.item = None

    def setAdjacent(self, direction: Direction, room: Room):
        """ Sets which room is located in a direction from the current room. Also sets this room's
            position in the adjacent room. """
        self.adjacentRooms[direction]            = room
        room.adjacentRooms[direction.opposite()] = self


def generateMap() -> Room:
    startingRoom = Room()
    #TODO.
    return startingRoom


def main():
    # Startup menu.
    startMenu = OptionSelector()
    startMenu.addOption('p', centerMessage("(P)lay"))
    startMenu.addOption('i', centerMessage("(I)nstructions"))
    startMenu.addOption('a', centerMessage("(A)bout"))
    startMenu.addOption('e', centerMessage("(E)xit"))

    while True:
        clearScreen()
        delayedPrint("BitMasher V4.3853256532", center=True)
        delayedPrint()
        delayedPrint("Type and enter the character in brackets to select an option.", center=True)
        delayedPrint()
        
        choice = startMenu.getSelection()
        if choice == 'e':
            exitGame()

        elif choice == 'i':
            clearScreen()
            delayedPrint("Instructions", center=True)
            delayedPrint()
            delayedPrint("TODO: add instructions.", center=True)
            delayedPrint()
            delayedPrint("Press ENTER to contiune.", center=True)
            input()

        elif choice == 'a':
            clearScreen()
            delayedPrint("About", center=True)
            delayedPrint()
            delayedPrint("\tAs part of one of my classes, I need to create a text-based adventure "
                         "game where you visit various rooms to gather items. If you get all the "
                         "items before you meet the boss, you win, else, you lose.")
            delayedPrint("\tThis class is far too low level for me, but it's still a requirement for "
                         "the degree. Thus, I have decided to massively overcomplicate said game and "
                         "make it something somewhat special. I can't stand going through the effort "
                         "of making something and doing it half-baked.")
            delayedPrint("\tI came up with the idea by thinking about what theme I should use, "
                         "picking the first idea, then adding any features that came to mind.")
            delayedPrint("\tAnyways, have fun.")
            delayedPrint()
            delayedPrint("Press ENTER to contiune.", center=True); input()

        elif choice == 'p':
            break
    
    # Gameloop.
    currentRoom = generateMap()

    while True:
        pass

if __name__ == '__main__':
    main()
