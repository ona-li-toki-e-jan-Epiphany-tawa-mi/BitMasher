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

////////////////////////////////////////////////////////////////////////////////
// Configuration                                                              //
////////////////////////////////////////////////////////////////////////////////

// The time each line printed with delayed_print() waits for in nanoseconds.
#define DELAYED_PRINT_DELAY_NS 110000000

////////////////////////////////////////////////////////////////////////////////
// Utilities                                                                  //
////////////////////////////////////////////////////////////////////////////////

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

static void sleep_ns(long nanoseconds) {
    struct timespec time = { .tv_sec = 0, .tv_nsec = nanoseconds };
    (void)nanosleep(&time, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// IO                                                                         //
////////////////////////////////////////////////////////////////////////////////

/**
 * Outputs an error and crashes if the stdin has EOF or an error.
 *
 * @param stream_name - the name of the stream for error messages.
 */
static void handle_stdin_error() {
    if (0 != feof(stdin)) {
        (void)fprintf(stderr, "ERROR: encountered EOF reading stdin\n");
        exit(1);
    } else if (0 != ferror(stdin)) {
        (void)fprintf(stderr, "ERROR: encountered error reading stdin\n");
        exit(1);
    }
}

/**
 * Do not call this directly. Call delayed_print() instead.
 *
 * Prints a string slice in the terminal with a delay.
 *
 * @param center - whether to print the slice centered in terminal. Will print
 * the text without centering if the slice is larger than the width.
 */
static void delayed_print_internal( bool center
                                  , const char* slice
                                  , size_t length) {
    assert(NULL != slice);

    if (center) {
        const Terminal* terminal      = terminal_size();
        size_t          remainder     = terminal->width - length;
        size_t          left_padding  = remainder / 2;
        size_t          right_padding = remainder - left_padding;

        if (length < terminal->width) {
            for (size_t i = 0; i < left_padding; ++i) (void)putchar(' ');
        }
        for (size_t i = 0; i < length; ++i) (void)putchar(slice[i]);
        if (length < terminal->width) {
            for (size_t i = 0; i < right_padding; ++i) (void)putchar(' ');
        }

    } else {
        for (size_t i = 0; i < length; ++i) (void)putchar(slice[i]);
        (void)putchar('\n');
    }

    (void)fflush(stdout);
    sleep_ns(DELAYED_PRINT_DELAY_NS);
}

/**
 * Prints a message line-by-line. Each line is printed with a delay to give an
 * old computer vibe.
 *
 * @param center - whether to print the lines centered in the terminal.
 */
static void delayed_print(bool center, const char* message) {
    assert(NULL != message);

    const Terminal* terminal = terminal_size();
    const char*     start   = message;
    const char*     end     = start;
    size_t          length  = 0;

    while ('\0' != *end) {
        ++length;

        // If newline, output slice.
        if ('\n' == *end) {
            delayed_print_internal(center, start, length - 1);
            (void)putchar('\n');
            start  = end + 1;
            end    = start;
            length = 0;
            continue;
        }

        // If current slice is too bit to be centered, output slice.
        if (length >= terminal->width) {
            delayed_print_internal(center, start, length);
            start  = end + 1;
            end    = start;
            length = 0;
            continue;
        }

        ++end;
    }

    // Print leftover slice.
    if (0 < length) delayed_print_internal(center, start, length);
}

/**
 * Prints a newline with a delay to give an old computer vibe.
 */
static void delayed_print_newline() {
    delayed_print(false, "\n");
}

static void clear() {
    // \x1B[2J - clears screen.
    // \x1B[H  - returns cursor to home position.
    fputs("\x1B[2J\x1B[H", stdout);
}

/**
 * Waits for the player to press enter, and outputs some text to notify them of
 * that.
 *
 * @param center - whether to center the notification to press enter.
 */
static void await_player(bool center) {
    static const char *const message = "Press ENTER to continue";
    delayed_print(center, message);

    while (true) {
        char input = (char)getchar();
        if (EOF == input) handle_stdin_error();
        if ('\n' == input) break;
    }
}

typedef struct {
    const char* data;
    bool        center;
} SelectorMessage;

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

    selector->options[selector->options_size++] = (char)tolower(option);
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
            handle_stdin_error();
        }

        char selection = '\0';
        for (char* c = buffer; '\0' != *c; ++c)
            if (!isspace(*c)) {
                selection = *c;
                break;
            }
        if ('\0' == selection) continue;

        selection = (char)tolower(selection);
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
// Items and Inventory                                                        //
////////////////////////////////////////////////////////////////////////////////

typedef enum {
    ITEM_FULL_MEMORY_READ_ACCESS = 0,
    ITEM_FULL_MEMORY_WRITE_ACCESS,
    ITEM_POINTER_DEREFERENCER,
    ITEM_OS_OVERRIDE_CAPABILITY,
    ITEM_RANSOMWARE_CODE_FRAGMENT,
    ITEM_VULNERABILITY,
    ITEM_SANDBOXER,
    ITEM_NONE,
    // The RANSOMWARE is stored on the map as an item since there is not going
    // to be an item in that room anyways.
    ITEM_RANSOMWARE
} ItemType;

static const char* item_type_name(ItemType type) {
    switch (type) {
    case ITEM_FULL_MEMORY_READ_ACCESS:  return "Full memory read access";
    case ITEM_FULL_MEMORY_WRITE_ACCESS: return "Full memory write access";
    case ITEM_POINTER_DEREFERENCER:     return "Pointer dereferencer";
    case ITEM_OS_OVERRIDE_CAPABILITY:   return "OS override capability";
    case ITEM_RANSOMWARE_CODE_FRAGMENT: return "RANSOMWARE code fragment";
    case ITEM_VULNERABILITY:            return "Vulnerability";
    case ITEM_SANDBOXER:                return "Sandboxer";
    case ITEM_NONE:                     return "None";
    case ITEM_RANSOMWARE:               return "The RANSOMWARE";
    default:                            assert(false && "unreachable");
    }
}

typedef struct {
    ItemType type;
    size_t   quantity;
} Item;

#define INVENTORY_MAX_SIZE 25
typedef struct {
    Item   items[INVENTORY_MAX_SIZE];
    size_t items_size;
} Inventory;

static void inventory_add_item(Inventory* inventory, Item item) {
    assert(NULL != inventory);

    if (ITEM_NONE == item.type) return;

    bool item_exists = false;
    for (size_t i = 0; i < inventory->items_size; ++i)
        if (item.type == inventory->items[i].type) {
            inventory->items[i].quantity += item.quantity;
            item_exists = true;
        }

    if (!item_exists) {
        assert(INVENTORY_MAX_SIZE >= inventory->items_size);
        inventory->items[inventory->items_size++] = item;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Game                                                                       //
////////////////////////////////////////////////////////////////////////////////

static Inventory generate_required_items() {
    Inventory items = {0};

    inventory_add_item(&items, (Item) {
        .type = ITEM_FULL_MEMORY_READ_ACCESS,
        .quantity = 1
    });
    inventory_add_item(&items, (Item) {
        .type = ITEM_FULL_MEMORY_WRITE_ACCESS,
        .quantity = 1
    });
    inventory_add_item(&items, (Item) {
        .type = ITEM_POINTER_DEREFERENCER,
        .quantity = 1
    });
    inventory_add_item(&items, (Item) {
        .type = ITEM_OS_OVERRIDE_CAPABILITY,
        .quantity = 1
    });
    inventory_add_item(&items, (Item) {
        .type = ITEM_SANDBOXER,
        .quantity = 1
    });
    inventory_add_item(&items, (Item) {
        .type = ITEM_RANSOMWARE_CODE_FRAGMENT,
        .quantity = 1 + (size_t)rand() % 3
    });
    inventory_add_item(&items, (Item) {
        .type = ITEM_VULNERABILITY,
        .quantity = 1 + (size_t)rand() % 3
    });

    return items;
}

static void run_game() {
    Inventory required_items = generate_required_items();

    (void)printf("Running the game!\n");
    exit(0);
}

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
    clear();

    delayed_print(true, "INSTRUCTIONS");
    delayed_print_newline();
    delayed_print(true, "You are an antivirus trying to rid a computer of a "
                         "RANSOMWARE before it takes over the system. There is "
                         "a finite amount of time before the system is fully "
                         "infected");
    delayed_print_newline();
    delayed_print(true, "In order to defeat it, you must find all items before "
                        "you find the RANSOMWARE. If you do not, you will not "
                        "be able to EXTRACT it and you will lose.");
    delayed_print_newline();
    delayed_print(true, "Each system (room) contains an item, which you can "
                        "move to; UP, DOWN, LEFT, AND RIGHT. Keep in mind that "
                        "the map is NOT 2D; Moving RIGHT, UP, LEFT, and DOWN "
                        "will lead to a different room than the one you "
                        "started in. The map is 'Spiky' so-to-speak.");
    delayed_print_newline();
    delayed_print(true, "You have a SCANner to aid in figuring out which rooms "
                        "contain items and which have RANSOMWARE. Using the "
                        "SCANner will reveal what the surronding rooms "
                        "contain, and the room you are currently in will be "
                        "automatically SCANned for you. But beware: SCANning "
                        "takes time. Also, occasionaly a SCAN will fail and "
                        "need to be repeated.");
    delayed_print_newline();
    delayed_print(true, "Good luck");
    delayed_print_newline();

    await_player(true);
}

static void run_about_menu() {
    clear();

    delayed_print(true, "ABOUT");
    delayed_print_newline();
    delayed_print(true, "As part of some garbage that doesn't matter, I needed "
                        "to create a text-based adventure game where you visit "
                        "various rooms to gather items. If you get all the "
                        "items before you meet the boss, you win, else, you "
                        "lose.");
    delayed_print_newline();
    delayed_print(true, "I had decided to massively overcomplicate said game "
                        "and make it something somewhat special. I can't "
                        "stand going through the effort of making something "
                        "and doing it half-baked.");
    delayed_print_newline();
    delayed_print(true, "Originally, this was written in Python, but I later "
                        "decided to rewrite it in C for funsies.");
    delayed_print_newline();
    delayed_print(true, "Anyways, have fun");
    delayed_print_newline();

    await_player(true);
}

static void run_liscense_menu() {
    clear();

    delayed_print(true, "LICENSE");
    delayed_print_newline();
    delayed_print(true, "Copyright (C) 2024 "
                        "ona-li-toki-e-jan-Epiphany-tawa-mi.");
    delayed_print_newline();
    delayed_print(true, "This program is free software: you can redistribute "
                        "it and/or modify it under the terms of the GNU "
                        "General Public License as published by the Free "
                        "Software Foundation, either version 3 of the License, "
                        "or (at your option) any later version.");
    delayed_print_newline();
    delayed_print(true, "This program is distributed in the hope that it will "
                        "be useful, but WITHOUT ANY WARRANTY; without even the "
                        "implied warranty of MERCHANTABILITY or FITNESS FOR A "
                        "PARTICULAR PURPOSE. See the GNU General Public "
                        "License for more details.");
    delayed_print_newline();
    delayed_print(true, "You should have received a copy of the GNU General "
                        "Public License along with this program. If not, see "
                        "http://www.gnu.org/licenses/.");
    delayed_print_newline();
    delayed_print(true, "Source (paltepuk):");
    delayed_print(true, "https://http://paltepuk.xyz/cgit/BitMasher.git/about/");
    delayed_print(true, "(I2P) http://oytjumugnwsf4g72vemtamo72vfvgmp4lfsf6wmggcvba3qmcsta.b32.i2p/cgit/BitMasher.git/about/");
    delayed_print(true, "(Tor) http://4blcq4arxhbkc77tfrtmy4pptf55gjbhlj32rbfyskl672v2plsmjcyd.onion/cgit/BitMasher.git/about/");
    delayed_print(true, "Source (GitHub):");
    delayed_print(true, "https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/BitMasher/");
    delayed_print_newline();

    await_player(true);
}

static void run_exit_sequence() {
    (void)printf("EXITing");
    (void)fflush(stdout);
    sleep_ns(DELAYED_PRINT_DELAY_NS);
    (void)printf(".");
    (void)fflush(stdout);
    sleep_ns(DELAYED_PRINT_DELAY_NS);
    (void)printf(".");
    (void)fflush(stdout);
    sleep_ns(DELAYED_PRINT_DELAY_NS);
    (void)printf(".\n");

    exit(0);
}

static void run_start_menu() {
    Selector start_menu = {0};
    selector_add_option(&start_menu, 'p', true, "(P)LAY");
    selector_add_option(&start_menu, 'i', true, "(I)NSTRUCTIONS");
    selector_add_option(&start_menu, 'a', true, "(A)BOUT");
    selector_add_option(&start_menu, 'l', true, "(L)ICENSE");
    selector_add_option(&start_menu, 'e', true, "(E)XIT");

    while (true) {
        clear();

        for (size_t line = 0; line < ARRAY_SIZE(logo); ++line) {
            delayed_print(true, logo[line]);
        }
        delayed_print_newline();
        delayed_print(true, version);
        delayed_print_newline();
        delayed_print(true, "Type and enter the character in paranthesis to "
                            "select an option.");
        delayed_print_newline();

        char choice = selector_get_selection(&start_menu);
        switch (choice) {
        case 'p': return;
        case 'i': run_instructions_menu(); break;
        case 'a': run_about_menu();        break;
        case 'l': run_liscense_menu();     break;
        case 'e': run_exit_sequence();     break;
        default:  assert(false && "unreachable");
        }
    }
}

int main(void) {
    if (!isatty(STDOUT_FILENO)) {
        (void)fprintf(stderr, "ERROR: stdout is not a terminal\n");
        return 1;
    }

    // Seed random number generator.
    srand((unsigned int)time(NULL));

    // Exits with exit().
    while (true) {
        run_start_menu();
        run_game();
    }

    return 0;
}
