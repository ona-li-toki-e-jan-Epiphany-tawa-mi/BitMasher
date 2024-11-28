/* This file is part of BitMasher.
 *
 * Copyright (c) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi
 *
 * BitMasher is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BitMasher is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * BitMasher. If not, see <https://www.gnu.org/licenses/>.
 */

/*  ______  __________________ _______  _______  _______           _______  _______
 * (  ___ \ \__   __/\__   __/(       )(  ___  )(  ____ \|\     /|(  ____ \(  ____ )
 * | (   ) )   ) (      ) (   | () () || (   ) || (    \/| )   ( || (    \/| (    )|
 * | (__/ /    | |      | |   | || || || (___) || (_____ | (___) || (__    | (____)|
 * |  __ (     | |      | |   | |(_)| ||  ___  |(_____  )|  ___  ||  __)   |     __)
 * | (  \ \    | |      | |   | |   | || (   ) |      ) || (   ) || (      | (\ (
 * | )___) )___) (___   | |   | )   ( || )   ( |/\____) || )   ( || (____/\| ) \ \__
 * |/ \___/ \_______/   )_(   |/     \||/     \|\_______)|/     \|(_______/|/   \__/
 *
 * BitMasher, a text adventure game where you act as an antivirus attempting to
 * rid a computer of a ransomware attack.
 */

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// POSIX.
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

// TODO document functions.

////////////////////////////////////////////////////////////////////////////////
// Configuration                                                              //
////////////////////////////////////////////////////////////////////////////////

// The time each line printed with delayed_print() waits for in nanoseconds.
#define DELAYED_PRINT_DELAY_NS 110000000

////////////////////////////////////////////////////////////////////////////////
// IO                                                                         //
////////////////////////////////////////////////////////////////////////////////

static void handle_read_error(FILE* stream, const char* stream_name) {
    assert(NULL != stream);
    assert(NULL != stream_name);

    if (0 != feof(stream)) {
        (void)fprintf( stderr
                     , "ERROR: encountered EOF reading %s\n"
                     , stream_name);
        exit(1);
    } else if (0 != ferror(stream)) {
        (void)fprintf( stderr
                     , "ERROR: encountered error reading %s\n"
                     , stream_name);
        exit(1);
    }
}

typedef struct {
    unsigned long width;
    unsigned long height;
} Terminal;

static const Terminal* terminal_size() {
    static Terminal terminal = {0};
    static bool dynamic_size = true;

    if (!dynamic_size) return &terminal;

    // Try ioctl().
    do {
        struct winsize window_size = {0};
        if (-1 == ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size)) break;
        terminal.width  = window_size.ws_col;
        terminal.height = window_size.ws_row;
        return &terminal;
    } while (false);
    // Try COLUMNS and LINES environment variables.
    do {
        char* end;
        char* lines = getenv("LINES");
        if (NULL == lines) break;
        unsigned long height = strtoul(lines, &end, 10);
        if ('\0' == *lines || '\0' != *end || ULONG_MAX == height) break;
        char* columns = getenv("COLUMNS");
        if (NULL == columns) break;
        unsigned long width = strtoul(columns, &end, 10);
        if ('\0' == *columns || '\0' != *end || ULONG_MAX == width) break;
        terminal.width  = width;
        terminal.height = height;
        dynamic_size    = false;
        return &terminal;
    } while (false);
    // Assume size.
    terminal.width  = 80;
    terminal.height = 24;
    dynamic_size    = false;
    return &terminal;
}

// TODO make work with multiple lines.
static void print_centered(const char* message) {
    assert(NULL != message);

    size_t size = strlen(message);
    const Terminal* terminal = terminal_size();

    if (size >= terminal->width) {
        (void)puts(message);
    } else {
        size_t remainder     = terminal->width - size;
        size_t left_padding  = remainder / 2;
        size_t right_padding = remainder - left_padding;

        for (size_t i = 0; i < left_padding; ++i) putchar(' ');
        (void)fputs(message, stdout);
        for (size_t i = 0; i < right_padding; ++i) putchar(' ');
    }
}

// TODO finish.
static void delayed_print(bool center, const char* message) {
    assert(NULL != message);
    (void)center;

    (void)fflush(stdout);
    struct timespec time = { .tv_sec  = 0, .tv_nsec = DELAYED_PRINT_DELAY_NS };
    (void)nanosleep(&time, NULL);

    if (center) {
        print_centered(message);
    } else {
        (void)puts(message);
    }
}

static void clear_screen() {
    // \x1B[2J - clears screen.
    // \x1B[H  - returns cursor to home position.
    fputs("\x1B[2J\x1B[H", stdout);
}

static void await_player(bool center) {
    delayed_print(center, "Press ENTER to continue");

    while (true) {
        char input = (char)getchar();
        if (EOF == input) handle_read_error(stdin, "stdin");

        if ('\n' == input) return;
    }
}

typedef struct {
    const char* data;
    bool        center;
} SelectorMessage;

// TODO: make case insensitive.
#define SELECTOR_OPTIONS_MAX_SIZE  25
#define SELECTOR_MESSAGES_MAX_SIZE 2*SELECTOR_OPTIONS_MAX_SIZE
typedef struct {
    SelectorMessage messages[SELECTOR_MESSAGES_MAX_SIZE];
    size_t          messages_size;
    char            options[SELECTOR_OPTIONS_MAX_SIZE];
    size_t          options_size;
} Selector;

static void selector_add_message( Selector*   selector
                                , bool        center
                                , const char* message) {
    assert(NULL != selector);
    assert(selector->messages_size <= SELECTOR_MESSAGES_MAX_SIZE);
    assert(NULL != message);

    SelectorMessage smessage = { .data = message, .center = center };
    selector->messages[selector->messages_size++] = smessage;
}

static void selector_add_option( Selector*   selector
                               , char        option
                               , bool        center_description
                               , const char* description) {
    assert(NULL != selector);
    assert(selector->options_size <= SELECTOR_OPTIONS_MAX_SIZE);
    assert(!isspace(option));
    assert(NULL != description);

    selector->options[selector->options_size++] = option;
    selector_add_message(selector, center_description, description);
}

static void selector_clear(Selector* selector) {
    assert(NULL != selector);

    selector->messages_size = 0;
    selector->options_size  = 0;
}

#define SELECTOR_GET_SELECTION_BUFFER_SIZE 50
static char selector_get_selection(const Selector* selector) {
    assert(NULL != selector);

    if (0 == selector->options_size) return '\0';

    for (size_t i = 0; i < selector->messages_size; ++i) {
        SelectorMessage message = selector->messages[i];
        delayed_print(message.center, message.data);
    }

    while (true) {
        static char buffer[SELECTOR_GET_SELECTION_BUFFER_SIZE] = {0};
        if (NULL == fgets(buffer, SELECTOR_GET_SELECTION_BUFFER_SIZE, stdin)) {
            handle_read_error(stdin, "stding");
        }

        char selection = '\0';
        for (char* c = buffer; '\0' != *c; ++c)
            if (!isspace(*c)) {
                selection = *c;
                break;
            }
        if ('\0' == selection) continue;

        bool valid_selection = false;
        for (size_t option = 0; option < selector->options_size; ++option)
            if (selection == selector->options[option]) {
                valid_selection = true;
                break;
            }
        if (!valid_selection) {
            (void)printf("ERROR: invalid option '%c'!\n", selection);
            continue;
        }

        return selection;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Game                                                                       //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Main Menu                                                                  //
////////////////////////////////////////////////////////////////////////////////

static const char* logo[] = {
    " ______  __________________ _______  _______  _______           _______  _______ ",
    "(  ___ \\ \\__   __/\\__   __/(       )(  ___  )(  ____ \\|\\     /|(  ____ \\(  ____ )",
    "| (   ) )   ) (      ) (   | () () || (   ) || (    \\/| )   ( || (    \\/| (    )|",
    "| (__/ /    | |      | |   | || || || (___) || (_____ | (___) || (__    | (____)|",
    "|  __ (     | |      | |   | |(_)| ||  ___  |(_____  )|  ___  ||  __)   |     __)",
    "| (  \\ \\    | |      | |   | |   | || (   ) |      ) || (   ) || (      | (\\ (   ",
    "| )___) )___) (___   | |   | )   ( || )   ( |/\\____) || )   ( || (____/\\| ) \\ \\__",
    "|/ \\___/ \\_______/   )_(   |/     \\||/     \\|\\_______)|/     \\|(_______/|/   \\__/"
};

static const char* version = "V6.327438247";

static void run_instructions_menu() {
    clear_screen();

    delayed_print(true, "INSTRUCTIONS");
    delayed_print(false, "\n");
    delayed_print( true
                 , "\tYou are an antivirus trying to rid a computer of a "
                   "RANSOMWARE before it takes over the system. There is a "
                   "finite amount of time before the system is fully infected");
    delayed_print(false, "\n");
    delayed_print( true
                 , "\tIn order to defeat it, you must find all items before "
                   "you find the RANSOMWARE. If you do not, you will not be "
                   "able to EXTRACT it and you will lose.");
    delayed_print(false, "\n");
    delayed_print( true
                 , "\tEach system (room) contains an item, which you can move "
                   "to; UP, DOWN, LEFT, AND RIGHT. Keep in mind that the map "
                   "is NOT 2D; Moving RIGHT, UP, LEFT, and DOWN will lead to a "
                   "different room than the one you started in. The map is "
                   "'Spiky' so-to-speak.");
    delayed_print(false, "\n");
    delayed_print(true
                 , "\tYou have a SCANner to aid in figuring out which rooms "
                   "contain items and which have RANSOMWARE. Using the SCANner "
                   "will reveal what the surronding rooms contain, and the "
                   "room you are currently in will be automatically SCANned "
                   "for you. But beware: SCANning takes time. Also, "
                   "occasionaly a SCAN will fail and need to be repeated.");
    delayed_print(false, "\n");
    delayed_print(true, "Good luck");
    delayed_print(false, "\n");

    await_player(true);
}

static void run_about_menu() {
    clear_screen();

    delayed_print(true, "ABOUT");
    delayed_print(false, "\n");
    delayed_print( true
                 , "\tAs part of some garbage that doesn't matter, I needed to "
                   "create a text-based adventure game where you visit various "
                   "rooms to gather items. If you get all the items before you "
                   "meet the boss, you win, else, you lose.");
    delayed_print(false, "\n");
    delayed_print( true
                 , "\tI had decided to massively overcomplicate said game and "
                   "make it something somewhat special. I can't stand going "
                   "through the effort of making something and doing it "
                   "half-baked.");
    delayed_print(false, "\n");
    delayed_print( true
                 , "\tOriginally, this was written in Python, but I later "
                   "decided to rewrite it in C for funsies.");
    delayed_print(false, "\n");
    delayed_print(true, "Anyways, have fun");
    delayed_print(false, "\n");

    await_player(true);
}

static void run_liscense_menu() {
    clear_screen();

    delayed_print(true, "LICENSE");
    delayed_print(false, "\n");
    delayed_print( true
                 , "Copyright (C) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi.");
    delayed_print(false, "\n");
    delayed_print( true
                 , "This program is free software: you can redistribute it "
                   "and/or modify it under the terms of the GNU General Public "
                   "License as published by the Free Software Foundation, "
                   "either version 3 of the License, or (at your option) any "
                   "later version.");
    delayed_print(false, "\n");
    delayed_print( true
                 , "This program is distributed in the hope that it will be "
                   "useful, but WITHOUT ANY WARRANTY; without even the implied "
                   "warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR "
                   "PURPOSE. See the GNU General Public License for more "
                   "details.");
    delayed_print(false, "\n");
    delayed_print( true
                 , "You should have received a copy of the GNU General Public "
                   "License along with this program. If not, see "
                   "http://www.gnu.org/licenses/.");
    delayed_print(false, "\n");

    await_player(true);
}

static void run_start_menu() {
    // TODO center messages.
    Selector start_menu = {0};
    selector_add_option(&start_menu, 'p', true, "(P)LAY");
    selector_add_option(&start_menu, 'i', true, "(I)NSTRUCTIONS");
    selector_add_option(&start_menu, 'a', true, "(A)BOUT");
    selector_add_option(&start_menu, 'l', true, "(L)ICENSE");
    selector_add_option(&start_menu, 'e', true, "(E)XIT");

    while (true) {
        clear_screen();

        for (size_t line = 0; line < ARRAY_SIZE(logo); ++line) {
            delayed_print(true, logo[line]);
        }
        delayed_print(false, "\n");
        delayed_print(true, version);
        delayed_print(false, "\n");
        delayed_print(true, "Type and enter the character in paranthesis to "
                            "select an option.");
        delayed_print(false, "\n");

        char choice = selector_get_selection(&start_menu);
        switch (choice) {
        case 'p': return;
        case 'i': run_instructions_menu(); break;
        case 'a': run_about_menu();        break;
        case 'l': run_liscense_menu();     break;
        case 'e': assert(false && "TODO: implement exiting game");
        default:  assert(false && "unreachable");
        }
    }
}

int main(void) {
    run_start_menu();
    assert(false && "TODO: implement game");
    return 0;
}
