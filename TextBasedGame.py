#!/usr/bin/env python3

"""
BitMasher, a text adventure game where you act as an antiviris attempting to rid a computer of a 
    ransomware attack.

@author Nathaniel Needham
"""

import os
from enum import Enum, auto
import random
from shutil import get_terminal_size
from sys import exit
from time import sleep
from typing import List



def centerMessage(message: str) -> str:
    """ Returns message with the spacing required to appear centered in the terminal. """
    return message.center(get_terminal_size().columns)

def delayedPrint(message: str='', end: str='\n', center: bool=False):
    """ Used to give a slow-scroll effect akin to old computers like the Commodore 64 and Apple II. """
    screenWidth = get_terminal_size().columns
    remaining = 0

    # Attempts to split text up line-by-line in the case of strings longer than a line.
    while True:
        sleep(0.15)
        messageChunk = message[remaining:remaining + screenWidth]
        print(messageChunk if not center else centerMessage(messageChunk), end='')

        remaining += screenWidth
        if remaining >= len(message):
            break

    print(end=end)

def clearScreen():
    """ Clears the terminal. """
    os.system('cls' if os.name=='nt' else 'clear')


class OptionSelector:
    """ Used to easily create selection menus where users are given a choice among a list of options.
        
        Use addOption() to set up the menu, and then use getSelection() to have the user select 
        something. """

    def __init__(self):
        self.options = []
        self.messages = []

    def addOption(self, characterCode: str, message: str):
        """ Adds a new option. The character code is what the user will type to select that option. 
        Put only a single character, will be made lowercase if possible. """
        self.options.append(characterCode[0].lower())
        self.messages.append(message)

    def dumpOptions(self):
        """ Removes all added options. """
        self.options.clear()
        self.messages.clear()

    def getSelection(self) -> str:
        """ Prompts the user with the options and has them make a selection and returns it. """
        for message in self.messages:
            delayedPrint(message)

        while True:
            choice = input().lower()

            # If the user entered nothing there's no real reason to throw an error.
            if choice == '':
                continue

            if choice[0] not in self.options:
                delayedPrint(f"Error: Invalid option '{choice}'!")
                continue

            return choice



class Fighter:
    """ Represents a fighter in a battle, complete with health, damage, and digital bloodlust. """
    def __init__(self, initialHealth: int, damage: int, isInvulnerable: bool):
        self.health         = initialHealth
        self.damage         = damage
        self.isInvulnerable = isInvulnerable

    def attack(self, victim: 'Fighter') -> int:
        """ Applies self's damage to the victim, reducing their health and returning the damage done. """
        if victim.isInvulnerable:
            return 0 

        victim.health -= self.damage
        return self.damage

    def isDead(self):
        return self.health <= 0

def doRansomwareBattle(requiredItemsLeft: List[str]):
    """ Plays out the turn-based fight against the ransomware. """
    def moveDelay():
        """ Applies a short delay and prints a newline, which is done before every move in turn-based
            combat to make it feel more like... combat. """
        sleep(0.7)
        delayedPrint()

    #TODO: set stats based on missing items.
    player = Fighter(100, 15, False)
    ransomware = Fighter(100, 10, requiredItemsLeft != 0)

    fightMenu = OptionSelector()
    fightMenu.addOption('a', "(A)ttack")
    fightMenu.addOption('r', "(R)un away")
    fightMenu.addOption('d', "Do a funny (d)ance")
    fightMenu.addOption('e', "(E)xit game")

    while True:
        clearScreen()
        delayedPrint("The Ransomware", center=True)
        delayedPrint()
        delayedPrint(f"Your health: {player.health}")
        delayedPrint(f"Ransomware's health: {ransomware.health}")
        delayedPrint()

        choice = fightMenu.getSelection()
        if choice == 'a':
            moveDelay()
            delayedPrint("You go on the attack...")
            moveDelay()
            damage = player.attack(ransomware)
            delayedPrint(f"You attack for {damage} health ({ransomware.health} remaining)")

            if ransomware.isDead():
                moveDelay()
                delayedPrint("You win!")
                delayedPrint()
                delayedPrint("Press ENTER to contiune"); input()
                break

        elif choice == 'r':
            moveDelay()
            delayedPrint("You try running away...")
            moveDelay()
            delayedPrint("But there is no exit")

        elif choice == 'd':
            moveDelay()
            delayedPrint("You do a funny dance...")
            moveDelay()
            damage = player.attack(player)
            delayedPrint(f"You accidentally hit yourself for {damage} health ({player.health} remaining)")

            if player.isDead():
                moveDelay()
                delayedPrint()
                delayedPrint("TODO: add easter egg here")
                delayedPrint("Well anyways you lost lol")
                delayedPrint()
                delayedPrint("Press ENTER to continue"); input()
                break

        elif choice == 'e':
            break
        
        moveDelay()
        delayedPrint("The ransomware goes on the attack...")
        moveDelay()
        damage = ransomware.attack(player)
        delayedPrint(f"You were attacked for {ransomware.damage} health ({player.health} remaining)")

        if player.isDead():
            moveDelay()
            delayedPrint("You lose!")
            delayedPrint()
            delayedPrint("Press ENTER to contiune"); input()
            break

        moveDelay()
        delayedPrint("Press ENTER to contiune"); input()



class Direction(Enum):
    """ Represents a physical direction in which to travel. """
    UP = auto()
    DOWN = auto()
    LEFT = auto()
    RIGHT = auto()

    #TODO: possibly make this use a dict.
    def opposite(self) -> 'Direction':
        """ Returns the direction opposite to the current one. """
        if   self is Direction.UP:   return Direction.DOWN
        elif self is Direction.DOWN: return Direction.UP
        elif self is Direction.LEFT: return Direction.RIGHT
        else:                        return Direction.LEFT
        
class System:
    """ Represents a system (room) within the game. """
    def __init__(self, name: str, item=None):
        self.name = name
        self.item = item

        self.adjacentRooms = { Direction.UP:    None,
                               Direction.DOWN:  None,
                               Direction.LEFT:  None,
                               Direction.RIGHT: None  }

    def __getitem__(self, direction: Direction) -> 'System':
        """ Returns the adjacent system in the given direction. """
        return self.adjacentRooms[direction]

    def __setitem__(self, direction: Direction, room: 'System'):
        """ Sets the adjacent system in the given direction. """
        self.adjacentRooms[direction] = room

    def setAdjacent(self, direction: Direction, room: 'System') -> 'System':
        """ Sets which system is located in a direction from the current one. Also sets this system's
            position in the adjacent one. """
        self[direction]            = room
        room[direction.opposite()] = self
        return self



def generateMap(requiredItems: List[str]) -> System:
    """ Generates a new game map with randomly placed systems populated with items and the 
        randsomeware. Returns the starting system. """
    #TODO: ensure the item pool is smaller or the same size as the room pool.
    startingSystem = System("The Boot Loader")
    itemPool = requiredItems.copy()
    systemPool = generateMap.systems.copy()

    random.shuffle(itemPool)
    itemPool.append("The Ransomware") # We append the ransomware after the items have been shuffled as
                                      #     it needs to be generated last to ensure that there is a 
                                      #     path to every item, that it is not blocked by it.
    random.shuffle(systemPool)

    # All requried items must be generated, but not all rooms, thus we iterate through each item and
    #   generate a room for it.
    for i in range(0, len(itemPool)):
        traverser = startingSystem
        previousDirection = None
        stepsLeft = 10

        #TODO Does not connect existing systems together except for the previous one, resulting in
        #   "spiky maps." Possibly fix.
        #TODO Make sure travereser generates all items; there may be the possiblity it never does one.
        while stepsLeft >= 0:
            stepsLeft -= 1

            if random.random() < 0.7:
                possibleDirections = [direction for direction in list(Direction) 
                                      if traverser[direction] is not None and
                                         direction is not previousDirection]
                if len(possibleDirections) == 0:
                    stepsLeft += 1
                    continue

                nextDirection = random.choice(possibleDirections)
                traverser = traverser[nextDirection]
                previousDirection = nextDirection.opposite()

            else:
                possibleDirections = [direction for direction in list(Direction) 
                                      if traverser[direction] is None]
                if len(possibleDirections) == 0:
                    stepsLeft += 1
                    continue

                traverser.setAdjacent(random.choice(possibleDirections)
                                    , System(systemPool[i], itemPool[i]))
                break

    return startingSystem

generateMap.systems = [ "The Registry",
                        "The Network interfaces",
                        "The Kernal",
                        "The Hard drive",         
                        "WebSurfer",
                        "PainterEX",
                        "BitMasher",
                        "The ilo li sina Interpreter",
                        "FreeWriter",
                        "PIMG",
                        "The Espresso Runtime Enviroment",
                        "SuperCAD",
                        "MacroDoi",
                        "Conway's Ivory Tower"             ]



def generateRequiredItems() -> List[str]:
    """ Generates a list of the items that must be gathered to defeat the ransomware. """
    return [ "Full memory read access",
             "Full memory write access",
             "Pointer dereferencer",
             "OS override capability"    ] + \
           [ "Malware code fragment"     ] * random.randint(1, 3) + \
           [ "Vulnerability"             ] * random.randint(1, 3)

def displayInventory(inventory: List[str], requiredItems: List[str]):
    """ Displays the items the player has and the items that still need to be collected. Will return
        once they decide to leave the inventory menu. """
    clearScreen()
    delayedPrint("Inventory:", center=True)
    delayedPrint()
    if len(inventory) == 0:
        delayedPrint("Empty...", center=True)
    else:
        for item in inventory:
            delayedPrint(f"- {item}", center=True)

    delayedPrint()
    delayedPrint("Remaining Items:", center=True)
    delayedPrint()
    if len(requiredItems) == 0:
        delayedPrint("Everything needed has been found...", center=True)
    else:
        for item in requiredItems:
            delayedPrint(f"- {item}", center=True)
    
    delayedPrint()
    delayedPrint("Press ENTER to continue", center=True); input()

def runGame():
    """ Initliazes and runs the game, interacting with the player. Returns when the player decides to
        leave or they fail/complete it. """
    requiredItems = generateRequiredItems()
    currentSystem = generateMap(requiredItems)
    gameMenu = OptionSelector()

    inventory = []

    while True:
        clearScreen()
        delayedPrint(currentSystem.name, center=True)
        delayedPrint()

        #TODO: make this some kind of variable or something more robust than checking the name.
        if currentSystem.item == "The Ransomware":
            doRansomwareBattle(requiredItems)
            break

        gameMenu.dumpOptions()
        #TODO: Generalize movement code if possible. Try to make dependent on names of direction enum.
        if currentSystem[Direction.UP] is not None:
            gameMenu.addOption('u', f"[{currentSystem[Direction.UP].name}] is (u)p above")
        if currentSystem[Direction.DOWN] is not None:
            gameMenu.addOption('d', f"[{currentSystem[Direction.DOWN].name}] is (d)own below")
        if currentSystem[Direction.LEFT] is not None:
            gameMenu.addOption('l', f"[{currentSystem[Direction.LEFT].name}] is to the (l)eft")
        if currentSystem[Direction.RIGHT] is not None:
            gameMenu.addOption('r', f"[{currentSystem[Direction.RIGHT].name}] is to the (r)ight")
        if currentSystem.item is not None:
            gameMenu.addOption('t', f"There is a [{currentSystem.item}]. (T)ake it?")

        gameMenu.addOption('i', 'Open the (I)nventory')
        gameMenu.addOption('e', '(E)xit game')

        choice = gameMenu.getSelection()

        if choice == 'u':
            currentSystem = currentSystem[Direction.UP]
        elif choice == 'd':
            currentSystem = currentSystem[Direction.DOWN]
        elif choice == 'l':
            currentSystem = currentSystem[Direction.LEFT]
        elif choice == 'r':
            currentSystem = currentSystem[Direction.RIGHT]

        elif choice == 't':
            inventory.append(currentSystem.item)
            requiredItems.remove(currentSystem.item)
            currentSystem.item = None

        elif choice == 'i':
            displayInventory(inventory, requiredItems)

        elif choice == 'e':
            return



def exitGame():
    """ Displays an exiting message and then exits. """
    delayedPrint("Exiting", end='')
    delayedPrint(".", end='')
    delayedPrint(".", end='')
    delayedPrint(".")

    exit(0)

def startMenu():
    """ Displays the start menu to the player. Player can exit the game from the menu. Returns when
        the user decides to play."""
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
            return



def main():
    # When the player exits a running game the start menu should come up, but when they exit from the
    #   start menu it closes this program, so we can just use an infinite loop.
    while True: 
        startMenu()
        runGame()
    
if __name__ == '__main__':
    main()
