#!/usr/bin/env python3

"""
BitMasher, a text adventure game where you act as an antiviris attempting to rid a computer of a 
    ransomware attack.

@author Nathaniel Needham
"""

import os
import random
from enum import Enum, auto
from shutil import get_terminal_size
from sys import exit
from time import sleep
from typing import Dict, Iterable, List, NoReturn, Tuple, Union



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
    options:  List[str]
    messages: List[str]

    def __init__(self):
        self.options = []
        self.messages = []

    def addOption(self, characterCode: str, message: str):
        """ Adds a new option. The character code is what the user will type to select that option. 
            Put only a single character, will be made lowercase if possible. Options and messages are 
            displayed in the order they are added."""
        self.options.append(characterCode[0].lower())
        self.messages.append(message)

    def addMessage(self, message: str):
        """ Adds a new message. Messages and options are displayed in the order they are added."""
        self.messages.append(message)

    def dumpOptions(self):
        """ Removes all added options and messages. """
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



class ItemType(Enum):
    """ Represents the various types of items that can be collected. Also is used to represent the 
        ransomware on the map."""
    FULL_MEMORY_READ_ACCESS  = "Full memory read access"
    FULL_MEMORY_WRITE_ACCESS = "Full memory write access"
    POINTER_DEREFERENCER     = "Pointer dereferencer"
    OS_OVERRIDE_CAPABILITY   = "OS override capability"
    RANSOMWARE_CODE_FRAGMENT = "Ransomware code fragment"
    VULNERABILITY            = "Vulnerability"
    RANSOMWARE               = '' # The ransomware is stored on the map as an item since there is not
                                  #     going to be an item in that room anyways.
    NONE                     = 'None'

class Inventory:
    """ Used to represent a set of items along with the amount of each item stored. """
    items: Dict[ItemType, int]

    def __init__(self):
        self.items = {}

    def addItem(self, item: ItemType, count: int=1):
        """ Adds the given item to the inventory. """
        try:
            self.items[item] += count
        except KeyError:
            self.items[item] = count

    def tryRemoveItem(self, item: ItemType, count: int=1) -> bool:
        """ Attempts to remove the given item from the inventory. If the item is not present or the 
            number to remove exceeds the amount stored within, this does nothing and returns false.
            If the items were removed, this returns true."""
        if not self.items or self.items[item] < count:
            return False

        self.items[item] -= count
        if self.items[item] == 0:
            del self.items[item]

        return True

    def __iter__(self) -> Iterable[Tuple[ItemType, int]]:
        """ Allows iterating through the items and their counts. """
        for each in self.items.items():
            yield each

    def toItemList(self) -> List[ItemType]:
        """ Takes all of the items and places them in a single list. Multiple items of the same type
            will be duplicated. """
        itemList = []
        for item, count in self:
            for i in range(0, count):
                itemList.append(item)

        return itemList

    def countItems(self) -> int:
        """ Returns the number of items present in the inventory. """
        counter = 0
        for count in self.items.values():
            counter += count

        return counter

    def isEmpty(self) -> bool:
        return not self.items



class Fighter:
    """ Represents a fighter in a battle, complete with health, damage, and digital bloodlust. """
    name:           str
    health:         int
    damage:         int
    isInvulnerable: bool

    def __init__(self, name: str, initialHealth: int, damage: int, isInvulnerable: bool=False):
        self.name = name
        self.health         = initialHealth
        self.damage         = damage
        self.isInvulnerable = isInvulnerable

    def attack(self, victim: 'Fighter') -> int:
        """ Applies self's damage to the victim, reducing their health and returning the damage done. """
        if victim.isInvulnerable:
            return 0 

        victim.health -= self.damage
        return self.damage

    def isDead(self) -> bool:
        return self.health <= 0

    def getDisplayableStatus(self) -> str:
        return "{}: {} hp, {} def, {} dmg".format(
                self.name, self.health, 9999 if self.isInvulnerable else 0, self.damage)

def doRansomwareBattle(requiredItemsLeft: Inventory):
    """ Plays out the turn-based fight against the ransomware. """
    def moveDelay():
        """ Applies a short delay and prints a newline, which is done before every move in turn-based
            combat to make it feel more like... combat. """
        sleep(0.7)
        delayedPrint()

    #TODO: set stats based on missing items.
    player = Fighter("You", 50, 15)
    ransomware = Fighter("The Ransomware", 50, 10, isInvulnerable=not requiredItemsLeft.isEmpty())

    fightMenu = OptionSelector()
    fightMenu.addOption('a', "(A)TTACK")
    fightMenu.addOption('r', "(R)UN away")
    fightMenu.addOption('d', "Do a funny (D)ANCE")
    fightMenu.addOption('e', "(E)XIT game")

    while True:
        clearScreen()
        delayedPrint("The Ransomware", center=True)
        delayedPrint()
        delayedPrint(player.getDisplayableStatus())
        delayedPrint(ransomware.getDisplayableStatus())
        delayedPrint()

        choice = fightMenu.getSelection()
        if choice == 'a':
            moveDelay()
            delayedPrint("You go on the attack...")
            moveDelay()
            damage = player.attack(ransomware)
            delayedPrint(f"You attack, dealing {damage} dmg ({ransomware.health} hp remaining)")

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
            delayedPrint(f"You accidentally hit yourself, dealing {damage} dmg ({player.health} hp remaining)")

            if player.isDead():
                moveDelay()
                delayedPrint()
                delayedPrint("TODO: add easter egg here")#TODO
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
        delayedPrint(f"You were attacked, dealing {ransomware.damage} dmg ({player.health} hp remaining)")

        if player.isDead():
            moveDelay()
            delayedPrint("You lose!")
            delayedPrint()
            delayedPrint("Press ENTER to contiune"); input()
            break

        moveDelay()
        delayedPrint("Press ENTER to contiune"); input()



class ScanResult(Enum):
    """ Represents the result of scanning a room to find what's inside."""
    EMPTY     = auto()
    ABNORMAL  = auto()
    SUSPICOUS = auto()
    ERROR     = auto()
    NONE      = auto()

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
    name:          str
    item:          ItemType
    scanResult:    ScanResult
    adjacentRooms: Dict[Direction, Union['System', None]]

    def __init__(self, name: str, item: ItemType=ItemType.NONE):
        self.name       = name
        self.item       = item
        self.scanResult = ScanResult.NONE

        self.adjacentRooms = { Direction.UP:    None,
                               Direction.DOWN:  None,
                               Direction.LEFT:  None,
                               Direction.RIGHT: None  }

    def __getitem__(self, direction: Direction) -> Union['System', None]:
        """ Returns the adjacent system in the given direction. """
        return self.adjacentRooms[direction]

    def __setitem__(self, direction: Direction, room: Union['System', None]):
        """ Sets the adjacent system in the given direction. """
        self.adjacentRooms[direction] = room

    def setAdjacent(self, direction: Direction, room: 'System'):
        """ Sets which system is located in a direction from the current one. Also sets this system's
            position in the adjacent one. """
        self[direction]            = room
        room[direction.opposite()] = self

    def __iter__(self) -> Iterable[Tuple[Direction, Union['System', None]]]:
        """ Allows iterating through the adjacent rooms and the directions they are in. """
        for each in self.adjacentRooms.items():
            yield each

    def tryScan(self, canFail: bool=True) -> ScanResult:
        """ Attemps to scan the system. Will overwrite previous result. Has small chance to fail if
            canFail is left true. The new result is also returned. """
        scanResult = None

        if canFail and random.random() <= 0.1: 
            scanResult = ScanResult.ERROR
        elif self.item is ItemType.RANSOMWARE: 
            scanResult = ScanResult.SUSPICOUS
        elif self.item is not ItemType.NONE: 
            scanResult = ScanResult.ABNORMAL
        else:                         
            scanResult = ScanResult.EMPTY

        self.scanResult = scanResult
        return scanResult

    def tryAppendScanResult(self, message: str) -> str:
        """ Appends text containing a human-readable scan result to the given message. Used to show
            the result when printing the current and nearby systems."""
        if self.scanResult is ScanResult.NONE:
            return message

        result = None
        if   self.scanResult is ScanResult.ERROR:     result = "[ERROR]"
        elif self.scanResult is ScanResult.SUSPICOUS: result = "Abnormal. Suspicous activity"
        elif self.scanResult is ScanResult.ABNORMAL:  result = "Abnormal"
        elif self.scanResult is ScanResult.EMPTY:     result = "Empty"

        return f"{message} (scan: {result})"



def generateMap(requiredItems: Inventory) -> System:
    """ Generates a new game map with randomly placed systems populated with items and the 
        randsomeware. Returns the starting system. """
    #TODO: ensure the item pool is smaller or the same size as the room pool.
    startingSystem = System("The Boot Loader")
    itemPool = requiredItems.toItemList()
    systemPool = generateMap.systems.copy()

    random.shuffle(itemPool)
    itemPool.append(ItemType.RANSOMWARE) # We append the ransomware after the items have been shuffled as
                                         #     it needs to be generated last to ensure that there is a 
                                         #     path to every item, that it is not blocked by it.
    random.shuffle(systemPool)

    lastItemIndex = None
    itemIndex = 0
    systemIndex = 0

    # All requried items must be generated, but not all rooms, thus we iterate through each item and
    #   generate a room for it.
    while itemIndex < len(itemPool):
        # If this index meets or exceeds the size of the system pool, i.e. there a more items then 
        #   systems, we need to reduce the amount of items required so we can fit them on the map.
        if systemIndex >= len(systemPool) - 1:
            # We only want the first unavalible item index to know which items will have to be removed.
            lastItemIndex = itemIndex
            # Forces the last system to be used for the ransomware, which is at the end of the item pool.
            itemIndex = len(itemPool) - 1

        traverser = startingSystem
        previousDirection = None
        stepsLeft = 10

        #TODO Does not connect existing systems together except for the previous one, resulting in
        #   "spiky maps." Possibly fix.
        while stepsLeft >= 0:
            stepsLeft -= 1

            if random.random() < 0.7:
                possibleDirections = [direction for direction in list(Direction) 
                                      if traverser[direction] is not None and
                                         direction is not previousDirection]
                if not possibleDirections:
                    stepsLeft += 1
                    continue

                nextDirection = random.choice(possibleDirections)
                traverser = traverser[nextDirection]
                previousDirection = nextDirection.opposite()

            else:
                possibleDirections = [direction for direction in list(Direction) 
                                      if traverser[direction] is None]
                if not possibleDirections:
                    stepsLeft += 1
                    continue

                traverser.setAdjacent(random.choice(possibleDirections)
                                    , System(systemPool[systemIndex], itemPool[itemIndex]))
                break


        itemIndex   += 1
        systemIndex += 1

    # Removes required items that a room could not be made avalible for.
    if lastItemIndex is not None:
        for i in range(lastItemIndex, len(itemPool) - 1): # No need to remove ransomware, so -1.
            requiredItems.tryRemoveItem(itemPool[i])

    return startingSystem

generateMap.systems = [ "The Registry",
                        "The Network interfaces",
                        "The Kernal",
                        "The Hard drive",         
                        "WebSurfer",                         # Not real. 
                        "PainterEX",                         # Not real.    
                        "BitMasher",                         # ;).
                        "The ilo li sina Interpreter",       # https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/ilo-li-sina
                        "FreeWriter",                        # Not real.
                        "PIMG",                              # Not real.
                        "The Espresso Runtime Enviroment",   # Not real.
                        "SuperCAD",                          # Not real.
                        "MacroDoi",                          # https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/MacroDoi
                        "Conway's Ivory Tower"             ] # https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/Conways-Ivory-Towery",                         



def generateRequiredItems() -> Inventory:
    """ Generates a list of the items that must be gathered to defeat the ransomware. """
    requiredItems = Inventory()
    
    requiredItems.addItem(ItemType.FULL_MEMORY_READ_ACCESS)
    requiredItems.addItem(ItemType.FULL_MEMORY_WRITE_ACCESS)
    requiredItems.addItem(ItemType.POINTER_DEREFERENCER)
    requiredItems.addItem(ItemType.OS_OVERRIDE_CAPABILITY)
    requiredItems.addItem(ItemType.RANSOMWARE_CODE_FRAGMENT, count=random.randint(1, 3))
    requiredItems.addItem(ItemType.VULNERABILITY,            count=random.randint(1, 3))

    return requiredItems

def displayInventory(inventory: Inventory, requiredItems: Inventory):
    """ Displays the items the player has and the items that still need to be collected. Will return
        once they decide to leave the inventory menu. """
    clearScreen()
    delayedPrint("Inventory:", center=True)
    delayedPrint()
    if inventory.isEmpty():
        delayedPrint("Empty...", center=True)
    else:
        for item, count in inventory:
            delayedPrint(f"- {item.value}: {count}", center=True)

    delayedPrint()
    delayedPrint("Remaining Items:", center=True)
    delayedPrint()
    if requiredItems.isEmpty():
        delayedPrint("Everything needed has been found...", center=True)
    else:
        for item, count in requiredItems:
            delayedPrint(f"- {item.value}: {count}", center=True)
    
    delayedPrint()
    delayedPrint("Press ENTER to continue", center=True); input()

def runGame():
    """ Initliazes and runs the game, interacting with the player. Returns when the player decides to
        leave or they fail/complete it. """
    requiredItems = generateRequiredItems()
    pregenRequiredItemsCount = requiredItems.countItems()
    currentSystem = generateMap(requiredItems)
    gameMenu = OptionSelector()

    inventory = Inventory()

    # Warning for partial map generation.
    if pregenRequiredItemsCount != requiredItems.countItems():
        clearScreen()
        delayedPrint("WARNING: Unable to generate enough systems!", center=True)
        delayedPrint("Could only place {} items from a pool of {}".format(
                requiredItems.countItems(), pregenRequiredItemsCount)
                   , center=True)
        delayedPrint("There are only {} systems avalible in total for generation".format(
                generateMap.systems)
                   , center=True)
        delayedPrint("Please notify the developer(s) so they can fix it", center=True)
        delayedPrint()
        delayedPrint("The game should still run fine, so feel free to continue playing", center=True)
        delayedPrint()
        delayedPrint("Press ENTER to continue", center=True); input()

    while True:
        if currentSystem.item is ItemType.RANSOMWARE:
            doRansomwareBattle(requiredItems)
            break # Once the battle is over, the player either won or lost, so the game can be ended.

        clearScreen()
        currentSystem.tryScan(canFail=False)
        delayedPrint(currentSystem.tryAppendScanResult(currentSystem.name), center=True)
        delayedPrint()

        gameMenu.dumpOptions()
        #TODO: Generalize movement code if possible. Try to make dependent on names of direction enum.
        if currentSystem[Direction.UP] is not None:
            gameMenu.addOption('u', currentSystem[Direction.UP].tryAppendScanResult(
                    f"[{currentSystem[Direction.UP].name}] is (U)P above"))
        if currentSystem[Direction.DOWN] is not None:
            gameMenu.addOption('d', currentSystem[Direction.DOWN].tryAppendScanResult(
                    f"[{currentSystem[Direction.DOWN].name}] is (D)OWN below"))
        if currentSystem[Direction.LEFT] is not None:
            gameMenu.addOption('l', currentSystem[Direction.LEFT].tryAppendScanResult(
                    f"[{currentSystem[Direction.LEFT].name}] is to the (L)EFT"))
        if currentSystem[Direction.RIGHT] is not None:
            gameMenu.addOption('r', currentSystem[Direction.RIGHT].tryAppendScanResult(
                    f"[{currentSystem[Direction.RIGHT].name}] is to the (R)IGHT"))
        if currentSystem.item is not ItemType.NONE:
            gameMenu.addOption('t', f"There is a [{currentSystem.item.value}]. (T)AKE it?")

        gameMenu.addMessage('')
        gameMenu.addOption('s', '(S)CAN the neighboring systems')
        gameMenu.addOption('i', 'Open the (I)NVENTORY')
        gameMenu.addOption('e', '(E)XIT game')

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
            inventory.addItem(currentSystem.item)
            requiredItems.tryRemoveItem(currentSystem.item)
            currentSystem.item = ItemType.NONE

        elif choice == 's':
            delayedPrint()
            delayedPrint("Scanning...")
            sleep(0.8)
            delayedPrint()

            for direction, system in currentSystem:
                if system is None: 
                    continue

                result = system.tryScan()
                systemDescription = None

                if result is ScanResult.ERROR:    
                     systemDescription = "[ERROR]. Warning: possible inconclusive search"
                elif result is ScanResult.SUSPICOUS: 
                    systemDescription = "something abnormal present with trace evidence of suspicous " \
                                        "activity. Warning: proceed with caution"
                elif result is ScanResult.ABNORMAL:  
                    systemDescription = "something abnormal present"
                elif result is ScanResult.EMPTY:     
                    systemDescription = "nothing of significance"

                delayedPrint(f"[{direction.name}]wise there is {systemDescription}")

            delayedPrint()
            delayedPrint("Press ENTER to continue"); input()

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
    startMenu.addOption('p', centerMessage("(P)LAY"))
    startMenu.addOption('i', centerMessage("(I)NSTRUCTIONS"))
    startMenu.addOption('a', centerMessage("(A)BOUT"))
    startMenu.addOption('e', centerMessage("(E)XIT"))

    while True:
        clearScreen()
        delayedPrint("BitMasher V4.3853256532", center=True) # Meaningless version number.
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



def main() -> NoReturn:
    # When the player exits a running game the start menu should come up, but when they exit from the
    #   start menu it closes this program, so we can just use an infinite loop.
    while True: 
        startMenu()
        runGame()
    
if __name__ == '__main__':
    main()
