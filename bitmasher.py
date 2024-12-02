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
# Game                                                                         #
##################################################...##############################

def runGame():
    """ Initliazes and runs the game, interacting with the player. Returns when
        the player decides to leave or they fail/complete it. """
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
