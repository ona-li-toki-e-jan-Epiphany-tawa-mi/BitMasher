#!/usr/bin/env python3

################################################################################
# MIT License                                                                  #
#                                                                              #
# Copyright (c) 2022,2024 ona-li-toki-e-jan-Epiphany-tawa-mi                   #
#                                                                              #
# Permission is hereby granted, free of charge, to any person obtaining a copy #
# of this software and associated documentation files (the "Software"), to     #
# deal in the Software without restriction, including without limitation the   #
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  #
# sell copies of the Software, and to permit persons to whom the Software is   #
# furnished to do so, subject to the following conditions:                     #
#                                                                              #
# The above copyright notice and this permission notice shall be included in   #
# all copies or substantial portions of the Software.                          #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  #
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS #
# IN THE SOFTWARE.                                                             #
################################################################################

"""
BitMasher, a text adventure game where you act as an antivirus attempting to rid a computer of a
    ransomware attack.
 ______  __________________ _______  _______  _______           _______  _______
(  ___ \ \__   __/\__   __/(       )(  ___  )(  ____ \|\     /|(  ____ \(  ____ )
| (   ) )   ) (      ) (   | () () || (   ) || (    \/| )   ( || (    \/| (    )|
| (__/ /    | |      | |   | || || || (___) || (_____ | (___) || (__    | (____)|
|  __ (     | |      | |   | |(_)| ||  ___  |(_____  )|  ___  ||  __)   |     __)
| (  \ \    | |      | |   | |   | || (   ) |      ) || (   ) || (      | (\ (
| )___) )___) (___   | |   | )   ( || )   ( |/\____) || )   ( || (____/\| ) \ \__
|/ \___/ \_______/   )_(   |/     \||/     \|\_______)|/     \|(_______/|/   \__/
"""

## Configuration Section START
# The time, in seconds, between printing each line that gives the slow scroll effect.
SLOW_SCROLL_DELAY = 0.11

# The amount of time the player is given per system generated.
SECONDS_PER_SYSTEM = 8
# The time, in seconds, it takes to SCAN the surrounding systems.
SCAN_TIME = 0.8
# The chance a SCAN will fail, given as a number between 0 and 1.
SCAN_FAIL_CHANCE = 0.1

# The number of steps the traverser can take before it gives up. Higher values means it's more likely to generate a room, but
#   loading times will have the potential to increase.
MAX_STEPS = 100
# The chance that the traverser choses to move to an existing room over finding a new one, given as a
#   number between 0 and 1. Make larger for spikier maps.
MOVE_CHANCE = 0.7

# The amount of time, in seconds, it takes for a move to happen in the battle.
BATTLE_MOVE_DELAY = 0.7
# Base health for all fighters.
FIGHTER_BASE_HEALTH = 50
# The addtional health points the RANSOMWARE gets per missing code fragment.
CODE_FRAGMENT_HEALTH_BOOST = 25
# Base damage for all fighters.
FIGHTER_BASE_DAMAGE = 10
# The additional damage points the player gets. Must be larger than or equal to 0 for the player to
#   win whatsoever.
PLAYER_DAMAGE_BOOST = 5
# The damage boost the RANSOMWARE gets per missing vulnerability.
VULNERABILITY_DAMAGE_BOOST = 10
## Configuration Section END



import os
import random
from enum import Enum, auto
from shutil import get_terminal_size
from sys import exit, stdout
from time import sleep, time_ns
from typing import Dict, Iterable, List, NoReturn, Tuple, Union

SECONDS_TO_NANOSECONDS = 1_000_000_000



def centerMessage(message: str) -> str:
    """ Returns message with the spacing required to appear centered in the terminal. """
    return message.center(get_terminal_size().columns)

def delayedPrint(message: str='', end: str='\n', center: bool=False):
    """ Used to give a slow-scroll effect akin to old computers like the Commodore 64 and Apple II. """
    screenWidth = get_terminal_size().columns
    remaining = 0

    # Attempts to split text up line-by-line in the case of strings longer than a line.
    while True:
        sleep(SLOW_SCROLL_DELAY)
        messageChunk = message[remaining:remaining + screenWidth]
        # Prints flush the buffer since all text needs to appear with each call for the delay effect to work.
        print(messageChunk if not center else centerMessage(messageChunk), end='', flush=True)

        remaining += screenWidth
        if remaining >= len(message):
            break

    print(end=end)

def clearScreen():
    """ Clears the terminal. """
    # We flush the output buffer before clearing because if there is any residual it could be outputted after the clear.
    stdout.flush()
    os.system('cls' if os.name=='nt' else 'clear')

def awaitPlayer(center: bool=False):
    """ Displays a message and waits for the player to chose to continue. """
    delayedPrint("Press ENTER to contiune", center=center)
    input()


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

    def addMessage(self, message: str=''):
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
            choice = input()
            sanatizedChoice = choice.strip().lower()

            # If the user entered nothing there's no real reason to throw an error.
            if not sanatizedChoice:
                continue

            if sanatizedChoice[0] not in self.options:
                delayedPrint(f"Error: Invalid option '{choice}'!")
                continue

            return sanatizedChoice



class ItemType(Enum):
    """ Represents the various types of items that can be collected. Also is used to represent the
        ransomware on the map."""
    FULL_MEMORY_READ_ACCESS  = "Full memory read access"
    FULL_MEMORY_WRITE_ACCESS = "Full memory write access"
    POINTER_DEREFERENCER     = "Pointer dereferencer"
    OS_OVERRIDE_CAPABILITY   = "OS override capability"
    RANSOMWARE_CODE_FRAGMENT = "RANSOMWARE code fragment"
    VULNERABILITY            = "Vulnerability"
    SANDBOXER                = "Sandboxer"
    RANSOMWARE               = "The RANSOMWARE" # The RANSOMWARE is stored on the map as an item since
                                                #   there is not going to be an item in that room
                                                #   anyways.
    NONE                     = "None"

    def name(self) -> str:
        """ Returns the name of the item. """
        return self.value

class Inventory:
    """ Used to represent a set of items along with the amount of each item stored. """
    items: Dict[ItemType, int]

    def __init__(self):
        self.items = {}

    def addItem(self, item: ItemType, count: int=1):
        """ Adds the given item to the INVENTORY. """
        try:
            self.items[item] += count
        except KeyError:
            self.items[item] = count

    def tryRemoveItem(self, item: ItemType, count: int=1) -> bool:
        """ Attempts to remove the given item from the INVENTORY. If the item is not present or the
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
        """ Returns the number of items present in the INVENTORY. """
        counter = 0
        for count in self.items.values():
            counter += count

        return counter

    def countItem(self, item: ItemType) -> int:
        """ Returns amount of the specified item present, returning 0 if it isn't """
        try:
            return self.items[item]
        except KeyError:
            return 0

    def isEmpty(self) -> bool:
        return not self.items

    def contains(self, item: ItemType) -> bool:
        """ Checks if an item is present within the INVENTORY, regardless of count. """
        try:
            self.items[item]
            return True
        except KeyError:
            return False



def randomCharacter():
    """ Generates a random printable ASCII character. """
    return chr(random.randint(0x21, 0x7E))

def garbleString(string: str):
    """ Randomly replaces characters within a string. """
    mutableString  = list(string)
    characterCount = len(mutableString)
    replacements   = random.randint(0, characterCount)

    for i in range(0, replacements):
        index                = random.randrange(0, characterCount)
        mutableString[index] = randomCharacter()

    return "".join(mutableString)

def annoyingCase(string: str):
    """ Randomly changes the case of letters in the string. """
    mutableString = list(string)

    for i in range(0, len(mutableString)):
        mutableString[i] = mutableString[i].lower() if random.randint(0, 1) == 1 else mutableString[i].upper()

    return "".join(mutableString)

def playLoseSequence(funny: bool=False):
    """ Displays the normal losing sequence when you die. """
    clearScreen()

    # Function to generate text fill the screen with.
    spamGenerator = randomCharacter if not funny else (lambda: ";)")

    for i in range(0, 15):
        print(end='\a') # Beeps. Not always avalible.
        for k in range(0, 1000):
            print(spamGenerator(), end='')

        # We need to flush so that all characters appear before clearing the
        # screen.
        stdout.flush()
        sleep(0.1)

    clearScreen()
    for i in range(0, random.randint(5, 10)):
        delayedPrint(garbleString("GAME OVER GAME OVER GAME OVER"), center=True)
    delayedPrint()
    delayedPrint(annoyingCase("All Your systems are belong to us"), center=True)
    delayedPrint()
    for i in range(0, random.randint(20, 40)):
        delayedPrint(";;;;;;;;)))))", end='')
    print()
    delayedPrint()
    awaitPlayer(center=True)

class Fighter:
    """ Represents a fighter in a battle, complete with health, damage, and digital bloodlust. """
    name:           str
    health:         int
    damage:         int

    def __init__(self, name: str, initialHealth: int, damage: int):
        self.name = name
        self.health         = initialHealth
        self.damage         = damage

    def attack(self, victim: 'Fighter') -> int:
        """ Applies self's damage to the victim, reducing their health and returning the damage done. """
        victim.health -= self.damage
        return self.damage

    def isDead(self) -> bool:
        return self.health <= 0

    def getDisplayableStatus(self) -> str:
        """ Returns a human readable status that shows name, hp, and damage. """
        return f"{self.name}: {self.health} hp, {self.damage} dmg"

def doRansomwareBattle(requiredItemsLeft: Inventory, loseTime: int):
    """ PLAYs out the turn-based fight against the RANSOMWARE. """
    def moveDelay():
        """ Applies a short delay and prints a newline, which is done before every move in turn-based
            combat to make it feel more like... combat. """
        sleep(BATTLE_MOVE_DELAY)
        delayedPrint()

    memoryAlterationCapablity = not requiredItemsLeft.contains(ItemType.FULL_MEMORY_READ_ACCESS) and \
                                not requiredItemsLeft.contains(ItemType.FULL_MEMORY_WRITE_ACCESS)
    adminPrivileges = not requiredItemsLeft.contains(ItemType.OS_OVERRIDE_CAPABILITY)
    dereferencer = not requiredItemsLeft.contains(ItemType.POINTER_DEREFERENCER)
    sandboxed = not requiredItemsLeft.contains(ItemType.SANDBOXER)

    player = Fighter("You", FIGHTER_BASE_HEALTH, FIGHTER_BASE_DAMAGE + PLAYER_DAMAGE_BOOST)
    ransomware = Fighter("The RANSOMWARE"
                       , FIGHTER_BASE_HEALTH + CODE_FRAGMENT_HEALTH_BOOST *
            requiredItemsLeft.countItem(ItemType.RANSOMWARE_CODE_FRAGMENT)
                       , FIGHTER_BASE_DAMAGE + VULNERABILITY_DAMAGE_BOOST *
            requiredItemsLeft.countItem(ItemType.VULNERABILITY))

    fightMenu = OptionSelector()
    fightMenu.addOption('x', "E(X)TRACT")
    fightMenu.addOption('n', "Do (N)OTHING")
    fightMenu.addOption('d', "Do a funny (D)ANCE")
    fightMenu.addMessage()
    fightMenu.addOption('e', "(E)XIT game")

    # Intro sequence.
    clearScreen()
    delayedPrint("The RANSOMWARE", center=True)
    delayedPrint()
    delayedPrint("You have located the RANSOMWARE infecting the computer", center=True)
    delayedPrint("EXTRACT it from the system as soon as possible", center=True)
    delayedPrint("There is no other option", center=True)
    delayedPrint()
    awaitPlayer(center=True)

    while True:
        if not sandboxed:
            if time_ns() >= loseTime:
                playLoseSequence()
                break

        clearScreen()
        delayedPrint("The RANSOMWARE", center=True)
        if not sandboxed: delayedPrint("Time left: {:.1F} second(s)".format(
                (loseTime - time_ns()) / SECONDS_TO_NANOSECONDS)
                                     , center=True)
        delayedPrint()
        delayedPrint(player.getDisplayableStatus())
        delayedPrint(ransomware.getDisplayableStatus())
        delayedPrint()

        choice = fightMenu.getSelection()
        if choice == 'x':
            moveDelay()
            delayedPrint("You attempt to EXTRACT the RANSOMWARE...")
            moveDelay()

            if not dereferencer:
                delayedPrint("Unable to locate relavent memory to alter; you lack the capabilities")

            elif not memoryAlterationCapablity:
                delayedPrint("Unable to alter relavent memory; you lack the capabilities")

            elif not adminPrivileges:
                delayedPrint("Memory alteration denied; you lack sufficent privileges")

            else:
                damage = player.attack(ransomware)
                delayedPrint(f"You complete partial code EXTRACTion, dealing {damage} dmg ({ransomware.health} hp remaining)")

                if ransomware.isDead():
                    moveDelay()
                    clearScreen()
                    delayedPrint("Congratulations", center=True)
                    delayedPrint()
                    delayedPrint("You have successfully EXTRACTed the RANSOMWARE", center=True)
                    delayedPrint()
                    awaitPlayer(center=True)
                    break

        elif choice == 'n':
            moveDelay()
            delayedPrint("You do absolutely NOTHING...")

        elif choice == 'd':
            moveDelay()
            delayedPrint("You attempt a funny DANCE...")
            moveDelay()
            damage = player.attack(player)
            delayedPrint("You are an antivirus, you have no means to DANCE")
            moveDelay()
            delayedPrint("In the process you corrupted your own data, dealing {} dmg ({} hp remaining)"
                    .format(damage, player.health))

            # Special end if player kills themself.
            if player.isDead():
                moveDelay()
                playLoseSequence(funny=True)
                break

        elif choice == 'e':
            break


        # RANSOMWARE attack sequence.
        moveDelay()
        delayedPrint("The RANSOMWARE attempts to deliver a payload...")
        moveDelay()
        damage = ransomware.attack(player)
        delayedPrint("You were hit with a viral payload, dealing {} dmg ({} hp remaining)".format(
                damage, player.health))

        if player.isDead():
            moveDelay()
            playLoseSequence()
            break

        moveDelay()
        awaitPlayer()



class ScanResult(Enum):
    """ Represents the result of SCANning a room to find what's inside."""
    EMPTY     = auto()
    ABNORMAL  = auto()
    SUSPICOUS = auto()
    ERROR     = auto()
    NONE      = auto()

class Direction(Enum):
    """ Represents a physical direction in which to travel. """
    UP    = auto()
    DOWN  = auto()
    LEFT  = auto()
    RIGHT = auto()

    def opposite(self) -> 'Direction':
        """ Returns the direction opposite to the current one. """
        if   self is Direction.UP:   return Direction.DOWN
        elif self is Direction.DOWN: return Direction.UP
        elif self is Direction.LEFT: return Direction.RIGHT
        else:                        return Direction.LEFT

class SystemType(Enum):
    """ Represents the various systems that can be visited. """
    BOOTLOADER                   = "The Bootloader"
    REGISTRY                     = "The Registry"
    NETWORK_INTERFACES           = "The Network interfaces"
    KERNAL                       = "The Kernal"
    HARD_DRIVE                   = "The Hard drive"
    WEBSURFER                    = "WebSurfer"                       # Not real.
    PAINTEREX                    = "PainterEX"                       # Not real.
    BITMASHER                    = "BitMasher"                       # ;).
    ILO_LI_SINA_INTERPRETER      = "The ilo li sina Interpreter"     # https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/ilo-li-sina
    FREEWRITER                   = "FreeWriter"                      # Not real.
    PIMG                         = "PIMG"                            # Not real.
    ESPRESSO_RUNTIME_ENVIROMENT  = "The Espresso Runtime Enviroment" # Not real.
    SUPERCAD                     = "SuperCAD"                        # Not real.
    MACRODOI                     = "MacroDoi"                        # https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/MacroDoi
    CONWAYS_IVORY_TOWER          = "Conway's Ivory Tower"            # https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/Conways-Ivory-Towery"
    RANDOM_INFORMATION_GENERATOR = "Random-Information-Generator"    # https://github.com/FatherVonTayvious/Random-Information-Generator

class System:
    """ Represents a system (room) within the game. """
    type:          SystemType
    item:          ItemType
    scanResult:    ScanResult
    adjacentRooms: Dict[Direction, Union['System', None]]

    def __init__(self, type: SystemType, item: ItemType=ItemType.NONE):
        self.type       = type
        self.item       = item
        self.scanResult = ScanResult.NONE

        self.adjacentRooms = { Direction.UP:    None,
                               Direction.DOWN:  None,
                               Direction.LEFT:  None,
                               Direction.RIGHT: None  }

    def name(self) -> str:
        """ Returns the name of the system. """
        return self.type.value

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

        if canFail and random.random() <= SCAN_FAIL_CHANCE:
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



def generateSystemPool() -> List[SystemType]:
    """ Generates the pool of systems that the map generator can pull from. """
    systemPool = list(SystemType)
    systemPool.remove(SystemType.BOOTLOADER)
    return systemPool

def generateMap(requiredItems: Inventory) -> System:
    """ Generates a new game map with randomly placed systems populated with items and the
        randsomeware. Returns the starting system. """
    startingSystem = System(SystemType.BOOTLOADER)
    itemPool = requiredItems.toItemList()
    systemPool = generateSystemPool()

    random.shuffle(itemPool)
    itemPool.append(ItemType.RANSOMWARE) # We append the RANSOMWARE after the items have been shuffled as
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
            # Forces the last system to be used for the RANSOMWARE, which is at the end of the item pool.
            itemIndex = len(itemPool) - 1

        traverser = startingSystem
        previousDirection = None
        stepsLeft = MAX_STEPS

        while stepsLeft >= 0:
            stepsLeft -= 1

            if random.random() < MOVE_CHANCE:
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

        # If the traverser was unable to place the item we need to remove it.
        if stepsLeft < 0:
            requiredItems.tryRemoveItem(itemPool[itemIndex])
            systemIndex -= 1 # We set back the system index so the system can still be used.

        itemIndex   += 1
        systemIndex += 1

    # Removes required items that a room could not be made avalible for.
    if lastItemIndex is not None:
        postgenRequiredItemsCount = requiredItems.countItems()

        for i in range(lastItemIndex, len(itemPool) - 1): # No need to remove RANSOMWARE, so -1.
            requiredItems.tryRemoveItem(itemPool[i])

        # Warning for partial map generation.
        clearScreen()
        delayedPrint("WARNING: Unable to generate enough systems!", center=True)
        delayedPrint("Could only place {} items from a pool of {}".format(
                requiredItems.countItems(), postgenRequiredItemsCount)
                    , center=True)
        delayedPrint("There are only {} systems avalible in total for generation".format(
                generateMap.systems)
                    , center=True)
        delayedPrint("Please notify the developer(s) so they can fix it", center=True)
        delayedPrint()
        delayedPrint("The game should still run fine, so feel free to continue PLAYing", center=True)
        delayedPrint()
        awaitPlayer(center=True)

    return startingSystem



def generateRequiredItems() -> Inventory:
    """ Generates a list of the items that must be gathered to defeat the RANSOMWARE. """
    requiredItems = Inventory()

    requiredItems.addItem(ItemType.FULL_MEMORY_READ_ACCESS)
    requiredItems.addItem(ItemType.FULL_MEMORY_WRITE_ACCESS)
    requiredItems.addItem(ItemType.POINTER_DEREFERENCER)
    requiredItems.addItem(ItemType.OS_OVERRIDE_CAPABILITY)
    requiredItems.addItem(ItemType.SANDBOXER)
    requiredItems.addItem(ItemType.RANSOMWARE_CODE_FRAGMENT, count=random.randint(1, 3))
    requiredItems.addItem(ItemType.VULNERABILITY,            count=random.randint(1, 3))

    return requiredItems

def displayInventory(inventory: Inventory, requiredItems: Inventory):
    """ Displays the items the player has and the items that still need to be collected. Will return
        once they decide to leave the INVENTORY menu. """
    clearScreen()
    delayedPrint("INVENTORY:", center=True)
    delayedPrint()
    if inventory.isEmpty():
        delayedPrint("Empty...", center=True)
    else:
        for item, count in inventory:
            delayedPrint(f"- {item.name()}: {count}", center=True)

    delayedPrint()
    delayedPrint("Remaining Items:", center=True)
    delayedPrint()
    if requiredItems.isEmpty():
        delayedPrint("Everything needed has been found...", center=True)
    else:
        for item, count in requiredItems:
            delayedPrint(f"- {item.name()}: {count}", center=True)

    delayedPrint()
    awaitPlayer(center=True)

def runGame():
    """ Initliazes and runs the game, interacting with the player. Returns when the player decides to
        leave or they fail/complete it. """
    requiredItems = generateRequiredItems()
    currentSystem = generateMap(requiredItems)
    gameMenu = OptionSelector()
    inventory = Inventory()
    loseTime = time_ns() + requiredItems.countItems() * SECONDS_PER_SYSTEM * SECONDS_TO_NANOSECONDS

    while True:
        currentTime = time_ns()
        if currentTime >= loseTime:
            playLoseSequence()
            break

        if currentSystem.item is ItemType.RANSOMWARE:
            doRansomwareBattle(requiredItems, loseTime)
            break # Once the battle is over, the player either won or lost, so the game can be ended.

        clearScreen()
        currentSystem.tryScan(canFail=False)
        delayedPrint(currentSystem.tryAppendScanResult(currentSystem.name()), center=True)
        delayedPrint("Time left: {:.1F} second(s)".format(
                (loseTime - currentTime) / SECONDS_TO_NANOSECONDS)
                   , center=True)
        delayedPrint()

        gameMenu.dumpOptions()

        if currentSystem[Direction.UP] is not None:
            gameMenu.addOption('u', currentSystem[Direction.UP].tryAppendScanResult(
                    f"[{currentSystem[Direction.UP].name()}] is (U)P above"))
        if currentSystem[Direction.DOWN] is not None:
            gameMenu.addOption('d', currentSystem[Direction.DOWN].tryAppendScanResult(
                    f"[{currentSystem[Direction.DOWN].name()}] is (D)OWN below"))
        if currentSystem[Direction.LEFT] is not None:
            gameMenu.addOption('l', currentSystem[Direction.LEFT].tryAppendScanResult(
                    f"[{currentSystem[Direction.LEFT].name()}] is to the (L)EFT"))
        if currentSystem[Direction.RIGHT] is not None:
            gameMenu.addOption('r', currentSystem[Direction.RIGHT].tryAppendScanResult(
                    f"[{currentSystem[Direction.RIGHT].name()}] is to the (R)IGHT"))
        if currentSystem.item is not ItemType.NONE:
            gameMenu.addOption('t', f"There is a [{currentSystem.item.name()}]. (T)AKE it?")

        gameMenu.addMessage()
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
            delayedPrint("SCANning...")
            sleep(SCAN_TIME)

            for _, system in currentSystem:
                if system is None:
                    continue

                system.tryScan()

        elif choice == 'i':
            displayInventory(inventory, requiredItems)

        elif choice == 'e':
            return



def exitGame():
    """ Displays an EXITing message and then EXITs. """
    delayedPrint("EXITing", end='')
    delayedPrint(".", end='')
    delayedPrint(".", end='')
    delayedPrint(".")

    exit(0)

logo = [" ______  __________________ _______  _______  _______           _______  _______ ",
        "(  ___ \ \__   __/\__   __/(       )(  ___  )(  ____ \|\     /|(  ____ \(  ____ )",
        "| (   ) )   ) (      ) (   | () () || (   ) || (    \/| )   ( || (    \/| (    )|",
        "| (__/ /    | |      | |   | || || || (___) || (_____ | (___) || (__    | (____)|",
        "|  __ (     | |      | |   | |(_)| ||  ___  |(_____  )|  ___  ||  __)   |     __)",
        "| (  \ \    | |      | |   | |   | || (   ) |      ) || (   ) || (      | (\ (   ",
        "| )___) )___) (___   | |   | )   ( || )   ( |/\____) || )   ( || (____/\| ) \ \__",
        "|/ \___/ \_______/   )_(   |/     \||/     \|\_______)|/     \|(_______/|/   \__/"  ]

def startMenu():
    """ Displays the start menu to the player. Player can EXIT the game from the menu. Returns when
        the user decides to PLAY."""
    startMenu = OptionSelector()
    startMenu.addOption('p', centerMessage("(P)LAY"))
    startMenu.addOption('i', centerMessage("(I)NSTRUCTIONS"))
    startMenu.addOption('a', centerMessage("(A)BOUT"))
    startMenu.addOption('e', centerMessage("(E)XIT"))

    while True:
        clearScreen()
        for line in logo:
            delayedPrint(line, center=True) # Meaningless version number.
        delayedPrint()
        delayedPrint("V5.74351224532", center=True) # Meaningless version number.
        delayedPrint()
        delayedPrint("Type and enter the character in brackets to select an option.", center=True)
        delayedPrint()

        choice = startMenu.getSelection()
        if choice == 'e':
            exitGame()

        elif choice == 'i':
            clearScreen()
            delayedPrint("INSTRUCTIONS", center=True)
            delayedPrint()
            delayedPrint("\tYou are an antivirus trying to rid a computer of a RANSOMWARE before it "
                         "takes over the system. There is a finite amount of time before the system "
                         "is fully infected")
            delayedPrint("\tIn order to defeat it, you must find all items before you find the "
                         "RANSOMWARE. If you do not, you will not be able to EXTRACT it and you will "
                         "lose.")
            delayedPrint("\tEach system (room) contains an item, which you can move to; UP, DOWN, LEFT"
                         ", AND RIGHT. Keep in mind that the map is NOT 2D; Moving RIGHT, UP, LEFT, and"
                         " DOWN will lead to a different room than the one you started in. The map is "
                         "'Spiky' so-to-speak.")
            delayedPrint("\tYou have a SCANner to aid in figuring out which rooms contain items and "
                         "which have RANSOMWARE. Using the SCANner will reveal what the surronding rooms"
                         " contain, and the room you are currently in will be automatically SCANned for "
                         "you. But beware: SCANning takes time. Also, occasionaly a SCAN will fail and "
                         "need to be repeated.")
            delayedPrint()
            delayedPrint("Good luck", center=True)
            delayedPrint()
            awaitPlayer(center=True)

        elif choice == 'a':
            clearScreen()
            delayedPrint("ABOUT", center=True)
            delayedPrint()
            delayedPrint("\tAs part of one of my classes, I need to create a text-based adventure "
                         "game where you visit various rooms to gather items. If you get all the "
                         "items before you meet the boss, you win, else, you lose.")
            delayedPrint("\tThis class is far too low level for me, but it's still a requirement for "
                         "the degree. Thus, I have decided to massively overcomplicate said game and "
                         "make it something somewhat special. I can't stand going through the effort "
                         "of making something and doing it half-baked.")
            delayedPrint("\tI came up with the idea by thinking ABOUT what theme I should use, "
                         "picking the first idea, then adding any features that came to mind.")
            delayedPrint()
            delayedPrint("Anyways, have fun", center=True)
            delayedPrint()
            awaitPlayer(center=True)

        elif choice == 'p':
            return



def main() -> NoReturn:
    # When the player EXITs a running game the start menu should come up, but when they EXIT from the
    #   start menu it closes this program, so we can just use an infinite loop.
    while True:
        startMenu()
        runGame()

if __name__ == '__main__':
    main()
