#!/usr/bin/env python3

'''
BitMasher, a text adventure game where you act as an antiviris attempting to rid a 
    computer of a ransomware attack.

@author Nathaniel Needham
'''

import time
import os
import shutil
import sys



def centerMessage(message):
    """ Returns message with the spacing required to appear centered in the terminal. """
    return message.center(shutil.get_terminal_size().columns)

def delayedPrint(message='', end='\n', center=False):
    """ Used to give a slow-scroll effect akin to old computers like the Commodore 64 
        and Apple II. """
    time.sleep(0.15)
    print(message if not center else centerMessage(message), end=end)

def clearScreen():
    """ Clears the terminal. """
    os.system('cls' if os.name=='nt' else 'clear')
    # NOTE: Consider adding in the following line. Will need to check if terminal supports 
    #   ANSI escape codes.
    # print("\033[H\033[2J", end="")


class OptionSelector:
    """ Used to easily create selection menus where users are given a choice among a list 
        of options.
        
        Use addOption() to set up the menu, and then use getSelection() to have the user
        Select something. """

    def __init__(self):
        self.options = []
        self.messages = []

    #TODO: add input checking on character code.
    def addOption(self, characterCode, message):
        """ Adds a new option. The character code is what the user will type to select
            that option. Put only a single character, will be made lowercase if possible. """
        self.options.append(characterCode.lower())
        self.messages.append(message)

    def getSelection(self):
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




def main():
    # Startup menu.
    while True:
        clearScreen()
        delayedPrint("BitMasher V4.3853256532", center=True)
        delayedPrint()
        delayedPrint("Type and enter the character in brackets to select an option.", center=True)
        delayedPrint()
        startMenu = OptionSelector()
        startMenu.addOption('p', centerMessage("(P)lay"))
        startMenu.addOption('i', centerMessage("(I)nstructions"))
        startMenu.addOption('a', centerMessage("(A)bout"))
        startMenu.addOption('e', centerMessage("(E)xit"))
        
        choice = startMenu.getSelection()
        if choice == 'e':
            exitGame()
        elif choice == 'i':
            clearScreen()
            delayedPrint("TODO: add instructions.")
            delayedPrint("Press ENTER to contiune.")
            input()
        elif choice == 'a':
            clearScreen()
            delayedPrint("TODO: add about section.")
            delayedPrint("Press ENTER to contiune.")
            input()
        elif choice == 'p':
            break



if __name__ == '__main__':
    main()
