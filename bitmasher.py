#!/usr/bin/env python3

# This file is part of BitMasher.
#
# Copyright (c) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi
#
# BitMasher is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# BitMasher is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# BitMasher. If not, see <https://www.gnu.org/licenses/>.

"""BitMasher, a text adventure game where you act as an antivirus attempting to rid
a computer of a ransomware attack.
"""
#  ______  __________________ _______  _______  _______           _______  _______
# (  ___ \ \__   __/\__   __/(       )(  ___  )(  ____ \|\     /|(  ____ \(  ____ )
# | (   ) )   ) (      ) (   | () () || (   ) || (    \/| )   ( || (    \/| (    )|
# | (__/ /    | |      | |   | || || || (___) || (_____ | (___) || (__    | (____)|
# |  __ (     | |      | |   | |(_)| ||  ___  |(_____  )|  ___  ||  __)   |     __)
# | (  \ \    | |      | |   | |   | || (   ) |      ) || (   ) || (      | (\ (
# | )___) )___) (___   | |   | )   ( || )   ( |/\____) || )   ( || (____/\| ) \ \__
# |/ \___/ \_______/   )_(   |/     \||/     \|\_______)|/     \|(_______/|/   \__/

import os
import random
from enum import Enum, auto
from shutil import get_terminal_size
from sys import exit, stdout
from time import sleep, time_ns
from typing import Dict, Iterable, List, NoReturn, Tuple, Union, cast

################################################################################
# Configuration                                                                #
################################################################################

# The amount of time the player is given per system generated.
SECONDS_PER_SYSTEM = 8
# The time, in seconds, it takes to SCAN the surrounding systems.
SCAN_TIME = 0.8
# The chance a SCAN will fail, given as a number between 0 and 1.
SCAN_FAIL_CHANCE = 0.1

# The amount of time, in seconds, it takes for a move to happen in the battle.
BATTLE_MOVE_DELAY = 0.7
# Base health for all fighters.
FIGHTER_BASE_HEALTH = 50
# The addtional health points the RANSOMWARE gets per missing code fragment.
CODE_FRAGMENT_HEALTH_BOOST = 25
# Base damage for all fighters.
FIGHTER_BASE_DAMAGE = 10
# The additional damage points the player gets. Must be larger than or equal to
# 0 for the player to win whatsoever.
PLAYER_DAMAGE_BOOST = 5
# The damage boost the RANSOMWARE gets per missing vulnerability.
VULNERABILITY_DAMAGE_BOOST = 10

################################################################################
# Inventory                                                                    #
################################################################################

class Inventory:
    """ Used to represent a set of items along with the amount of each item
        stored. """
    items: Dict[ItemType, int]

    def __iter__(self) -> Iterable[Tuple[ItemType, int]]:
        """ Allows iterating through the items and their counts. """
        for each in self.items.items():
            yield each

    def toItemList(self) -> List[ItemType]:
        """ Takes all of the items and places them in a single list. Multiple
            items of the same type will be duplicated. """
        itemList = []
        for item, count in self: # type: ignore # False positive about __next__.
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
        """ Returns amount of the specified item present, returning 0 if it
            isn't """
        try:
            return self.items[item]
        except KeyError:
            return 0

    def isEmpty(self) -> bool:
        return not self.items

    def contains(self, item: ItemType) -> bool:
        """ Checks if an item is present within the INVENTORY, regardless of
            count. """
        try:
            self.items[item]
            return True
        except KeyError:
            return False

################################################################################
# Battle                                                                       #
################################################################################

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
        if random.randint(0, 1) == 1:
            mutableString[i] = mutableString[i].lower()
        else:
            mutableString[i] = mutableString[i].upper()

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
    """ Represents a fighter in a battle, complete with health, damage, and
        digital bloodlust. """
    name:           str
    health:         int
    damage:         int

    def __init__(self, name: str, initialHealth: int, damage: int):
        self.name = name
        self.health         = initialHealth
        self.damage         = damage

    def attack(self, victim: 'Fighter') -> int:
        """ Applies self's damage to the victim, reducing their health and
            returning the damage done. """
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
        """ Applies a short delay and prints a newline, which is done before
            every move in turn-based combat to make it feel more like...
            combat. """
        sleep(BATTLE_MOVE_DELAY)
        delayedPrint()

    memoryAlterationCapablity = not requiredItemsLeft.contains(ItemType.FULL_MEMORY_READ_ACCESS) and \
                                not requiredItemsLeft.contains(ItemType.FULL_MEMORY_WRITE_ACCESS)
    adminPrivileges = not requiredItemsLeft.contains(ItemType.OS_OVERRIDE_CAPABILITY)
    dereferencer = not requiredItemsLeft.contains(ItemType.POINTER_DEREFERENCER)
    sandboxed = not requiredItemsLeft.contains(ItemType.SANDBOXER)

    player = Fighter(
        "You",
        FIGHTER_BASE_HEALTH,
        FIGHTER_BASE_DAMAGE + PLAYER_DAMAGE_BOOST
    )
    ransomware = Fighter(
        "The RANSOMWARE"
        , FIGHTER_BASE_HEALTH + CODE_FRAGMENT_HEALTH_BOOST
          * requiredItemsLeft.countItem(ItemType.RANSOMWARE_CODE_FRAGMENT)
        , FIGHTER_BASE_DAMAGE + VULNERABILITY_DAMAGE_BOOST
          * requiredItemsLeft.countItem(ItemType.VULNERABILITY)
    )

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
    delayedPrint(
        "You have located the RANSOMWARE infecting the computer",
        center=True
    )
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
        if not sandboxed: delayedPrint(
                "Time left: {:.1F} second(s)".format(
                    (loseTime - time_ns()) / SECONDS_TO_NANOSECONDS)
                , center=True
        )
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
                    delayedPrint(
                        "You have successfully EXTRACTed the RANSOMWARE",
                        center=True
                    )
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
            delayedPrint("In the process you corrupted your own data, dealing {} dmg ({} hp remaining)".format(
                damage, player.health))

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

################################################################################
# Map/System                                                                   #
################################################################################

class ScanResult(Enum):
    """ Represents the result of SCANning a room to find what's inside."""
    EMPTY     = auto()
    ABNORMAL  = auto()
    SUSPICOUS = auto()
    ERROR     = auto()
    NONE      = auto()

class System:
    """ Represents a system (room) within the game. """
    scanResult:    ScanResult

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
        """ Sets which system is located in a direction from the current
            one. Also sets this system's position in the adjacent one. """
        self[direction]            = room
        room[direction.opposite()] = self

    def __iter__(self) -> Iterable[Tuple[Direction, Union['System', None]]]:
        """ Allows iterating through the adjacent rooms and the directions they
            are in. """
        for each in self.adjacentRooms.items():
            yield each

    def tryScan(self, canFail: bool=True) -> ScanResult:
        """ Attemps to scan the system. Will overwrite previous result. Has
            small chance to fail if canFail is left true.

            The new result is also returned. """
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
        """ Appends text containing a human-readable scan result to the given
            message. Used to show the result when printing the current and
            nearby systems."""
        if self.scanResult is ScanResult.NONE:
            return message

        result = None
        if   self.scanResult is ScanResult.ERROR:     result = "[ERROR]"
        elif self.scanResult is ScanResult.SUSPICOUS: result = "Abnormal. Suspicous activity"
        elif self.scanResult is ScanResult.ABNORMAL:  result = "Abnormal"
        elif self.scanResult is ScanResult.EMPTY:     result = "Empty"

        return f"{message} (scan: {result})"

def generateMap(requiredItems: Inventory) -> System:
    """ Generates a new game map with randomly placed systems populated with
        items and the randsomeware. Returns the starting system. """
    lastItemIndex = None
    itemIndex = 0
    systemIndex = 0

    # Removes required items that a room could not be made avalible for.
    if lastItemIndex is not None:
        postgenRequiredItemsCount = requiredItems.countItems()

        # No need to remove RANSOMWARE, so -1.
        for i in range(lastItemIndex, len(itemPool) - 1):
            requiredItems.tryRemoveItem(itemPool[i])



    return startingSystem

################################################################################
# Game                                                                         #
################################################################################

def displayInventory(inventory: Inventory, requiredItems: Inventory):
    """ Displays the items the player has and the items that still need to be
        collected. Will return once they decide to leave the INVENTORY menu. """
    clearScreen()
    delayedPrint("INVENTORY:", center=True)
    delayedPrint()
    if inventory.isEmpty():
        delayedPrint("Empty...", center=True)
    else:
        for item, count in inventory: # type: ignore # False positive about __next__.
            delayedPrint(f"- {item.name}: {count}", center=True)

    delayedPrint()
    delayedPrint("Remaining Items:", center=True)
    delayedPrint()
    if requiredItems.isEmpty():
        delayedPrint("Everything needed has been found...", center=True)
    else:
        for item, count in requiredItems: # type: ignore # False positive about __next__.
            delayedPrint(f"- {item.name}: {count}", center=True)

    delayedPrint()
    awaitPlayer(center=True)

def runGame():
    """ Initliazes and runs the game, interacting with the player. Returns when
        the player decides to leave or they fail/complete it. """
    currentSystem = generateMap(requiredItems)
    gameMenu = OptionSelector()
    inventory = Inventory()
    loseTime = time_ns() + requiredItems.countItems() * SECONDS_PER_SYSTEM \
        * SECONDS_TO_NANOSECONDS

    while True:
        currentTime = time_ns()
        if currentTime >= loseTime:
            playLoseSequence()
            break

        if currentSystem.item is ItemType.RANSOMWARE:
            doRansomwareBattle(requiredItems, loseTime)
            # Once the battle is over, the player either won or lost, so the
            # game can be ended.
            break

        clearScreen()
        currentSystem.tryScan(canFail=False)
        delayedPrint(
            currentSystem.tryAppendScanResult(currentSystem.name()),
            center=True
        )
        delayedPrint(
            "Time left: {:.1F} second(s)".format(
                (loseTime - currentTime) / SECONDS_TO_NANOSECONDS)
            , center=True
        )
        delayedPrint()

        gameMenu.dumpOptions()

        if currentSystem[Direction.UP] is not None:
            gameMenu.addOption(
                'u',
                currentSystem[Direction.UP].tryAppendScanResult(
                    f"[{currentSystem[Direction.UP].name()}] is (U)P above")
            )
        if currentSystem[Direction.DOWN] is not None:
            gameMenu.addOption(
                'd',
                currentSystem[Direction.DOWN].tryAppendScanResult(
                    f"[{currentSystem[Direction.DOWN].name()}] is (D)OWN below")
            )
        if currentSystem[Direction.LEFT] is not None:
            gameMenu.addOption(
                'l',
                currentSystem[Direction.LEFT].tryAppendScanResult(
                    f"[{currentSystem[Direction.LEFT].name()}] is to the (L)EFT")
            )
        if currentSystem[Direction.RIGHT] is not None:
            gameMenu.addOption(
                'r',
                currentSystem[Direction.RIGHT].tryAppendScanResult(
                    f"[{currentSystem[Direction.RIGHT].name()}] is to the (R)IGHT")
            )
        if currentSystem.item is not ItemType.NONE:
            gameMenu.addOption(
                't',
                f"There is a [{currentSystem.item.name()}]. (T)AKE it?"
            )

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

def main() -> NoReturn:
    while True:
        runGame()

if __name__ == '__main__':
    main()
