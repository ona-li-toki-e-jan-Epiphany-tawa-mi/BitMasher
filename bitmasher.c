/* This file is part of BitMasher.
 *
 * Copyright (c) 2024-2025 ona-li-toki-e-jan-Epiphany-tawa-mi
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

// https://github.com/cpredef/predef
#if defined(MSDOS) || defined(__MSDOS__) || defined(_MSDOS) || defined(__DOS__)
#  define PLATFORM_DOS
#else // defined(MSDOS) || defined(__MSDOS__) || defined(_MSDOS) || defined(__DOS__)
// Assume POSIX.
#  define PLATFORM_POSIX
#endif

#ifdef PLATFORM_POSIX
#  define _POSIX_C_SOURCE 199309L
#endif // PLATFORM_POSIX
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef PLATFORM_DOS
#  include <dos.h>
#endif // PLATFORM_DOS
#ifdef PLATFORM_POSIX
#  include <sys/ioctl.h>
#  include <time.h>
#  include <unistd.h>
#endif // PLATFORM_POSIX
// From include/.
#include <anal.h>

////////////////////////////////////////////////////////////////////////////////
// Configuration                                                              //
////////////////////////////////////////////////////////////////////////////////

// The amount of time the player is given per system generated.
#define SECONDS_PER_SYSTEM 8

// The time, in nanoseconds, it takes to SCAN the surrounding systems.
#define SCAN_TIME_NS 800000000L
// The chance a SCAN will fail.
// Must be 0 <= SCAN_FAIL_CHANCE < 100.
#define SCAN_FAIL_CHANCE 10

// The time each line printed with delayed_print() waits for in nanoseconds.
#define DELAYED_PRINT_DELAY_NS 110000000L

// The number of steps the traverser can take before it gives up. Higher values
// means it's more likely to generate a room, but loading times will have the
// potential to increase.
#define MAX_STEPS 100
// The chance that the traverser choses to move to an existing room over findinga
// a new one. Make larger for spikier maps.
// Must be 0 <= MOVE_CHANCE < 100
#define MOVE_CHANCE 70

// Amount of time, in nanoseconds, it takes for a move to happen in the battle.
#define BATTLE_MOVE_DELAY 700000000L
// Base health for all fighters.
#define FIGHTER_BASE_HEALTH 50
// Addtional health points the RANSOMWARE gets per missing code fragment.
#define CODE_FRAGMENT_HEALTH_BOOST 25
// Base damage for all fighters.
#define FIGHTER_BASE_DAMAGE 10
// Additional damage points the player gets.
// Must be > 0 for the player to win.
#define PLAYER_DAMAGE_BOOST 5
// Damage boost the RANSOMWARE gets per missing vulnerability.
#define VULNERABILITY_DAMAGE_BOOST 10

////////////////////////////////////////////////////////////////////////////////
// Utilities                                                                  //
////////////////////////////////////////////////////////////////////////////////

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

// Zero-initialized.
typedef struct {
    unsigned long width;
    unsigned long height;
} Terminal;

static const Terminal* terminal_size(void) {
    static Terminal terminal = {0};
    static bool dynamic_size = true;

    if (!dynamic_size) return &terminal;

#ifdef PLATFORM_POSIX
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
#endif // PLATFORM_POSIX
    // Assume size.
    terminal.width  = 80;
    terminal.height = 24;
    dynamic_size    = false;
    return &terminal;
}

/**
 * Sleeps for the specified number of nanoseconds.
 * @param nanoseconds - must be 0 <= nanoseconds < 1,000,000,000.
 */
static void sleep_ns(const long int nanoseconds) {
    assert(1000000000 > nanoseconds && 0 <= nanoseconds);
#ifdef PLATFORM_DOS
    delay((unsigned int)nanoseconds / 1000000);
#elif defined(PLATFORM_POSIX) // PLATFORM_DOS
    const struct timespec time = { .tv_sec = 0, .tv_nsec = nanoseconds };
    nanosleep(&time, NULL);
#else // PLATFORM_POSIX
#  error Unsupported platform
#endif
}

/**
 * Gets the current time, in seconds, since some unspecified point in the past.
 */
static long get_time_s(void) {
#ifdef PLATFORM_DOS
    struct time time = {0};
    gettime(&time);
    return time.ti_sec + 60 * (time.ti_min + (60 * time.ti_hour));
#elif defined(PLATFORM_POSIX) // PLATFORM_DOS
    struct timespec timespec = {0};
    if (-1 == clock_gettime(CLOCK_MONOTONIC, &timespec)) {
        perror("Failed to read from monotonic clock");
        exit(1);
    }
    return timespec.tv_sec;
#else // PLATFORM_POSIX
#  error Unsupported platform
#endif
}

////////////////////////////////////////////////////////////////////////////////
// IO                                                                         //
////////////////////////////////////////////////////////////////////////////////

/**
 * Outputs an error and crashes if the stdin has EOF or an error.
 *
 * @param stream_name - the name of the stream for error messages.
 */
static void handle_stdin_error(void) {
    if (0 != feof(stdin)) {
        fprintf(stderr, "ERROR: encountered EOF reading stdin\n");
        exit(1);
    } else if (0 != ferror(stdin)) {
        fprintf(stderr, "ERROR: encountered error reading stdin\n");
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
NONNULL static void delayed_print_internal(
    const bool        center,
    const char *const slice,
    const size_t      length
) {
    assert(NULL != slice);

    if (center) {
        const Terminal* terminal      = terminal_size();
        const size_t    remainder     = terminal->width - length;
        const size_t    left_padding  = remainder / 2;
        const size_t    right_padding = remainder - left_padding;

        if (length < terminal->width) {
            for (size_t i = 0; i < left_padding; ++i) putchar(' ');
        }
        for (size_t i = 0; i < length; ++i) putchar(slice[i]);
        if (length < terminal->width) {
            for (size_t i = 0; i < right_padding; ++i) putchar(' ');
        }

    } else {
        for (size_t i = 0; i < length; ++i) putchar(slice[i]);
        putchar('\n');
    }

    fflush(stdout);
    sleep_ns(DELAYED_PRINT_DELAY_NS);
}

#define DELAYED_PRINT_MAX_BUFFER_SIZE 512
/**
 * Prints a message line-by-line. Each line is printed with a delay to give an
 * old computer vibe.
 *
 * @param center - whether to print the lines centered in the terminal.
 * @param format - like with the printf function family.
 */
NONNULL static void PRINTF_TYPECHECK(2, 3) delayed_print(
    const bool        center,
    const char* const format,
    ...
) {
    assert(NULL != format);

    // Use vsnprintf to handle the format string.
    static char buffer[DELAYED_PRINT_MAX_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    const int result =
        vsnprintf(buffer, DELAYED_PRINT_MAX_BUFFER_SIZE, format, args);
    if (0 > result) {
        fprintf(
            stderr,
            "ERROR: "__FILE__":%s: failed to run vsnprintf: %s\n"
            , __func__, strerror(errno)
        );
        exit(1);
    } else if (result >= DELAYED_PRINT_MAX_BUFFER_SIZE) {
        fprintf(
            stderr,
            "WARN: "__FILE__":%s: output truncated when running vsnprintf with "
            "format '%s'\n",
            __func__, format
        );
    }
    va_end(args);

    const Terminal *const terminal = terminal_size();
    const char*           start    = buffer;
    const char*           end      = start;
    size_t                length   = 0;

    while ('\0' != *end) {
        ++length;

        // If newline, output slice.
        if ('\n' == *end) {
            delayed_print_internal(center, start, length - 1);
            putchar('\n');
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
static void delayed_print_newline(void) {
    delayed_print(false, "\n");
}

/**
 * Clears the terminal.
 */
static void clear(void) {
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
static void await_player(const bool center) {
    static const char *const message = "Press ENTER to continue";
    delayed_print(center, message);

    while (true) {
        const char input = (char)getchar();
        if (EOF == input) handle_stdin_error();
        if ('\n' == input) break;
    }
}

#define SELECTOR_OPTIONS_MAX_COUNT  25
// Zero-initialized.
typedef struct {
    char   options[SELECTOR_OPTIONS_MAX_COUNT];
    size_t count;
} Selector;

/**
 * @param option - the character the player must enter to select this option.
 * Must not be whitespace.
 */
NONNULL static void selector_add_option(Selector *const selector, const char option) {
    assert(NULL != selector);
    assert(!isspace(option));

#ifndef NDEBUG
    for (size_t i = 0; i < selector->count; ++i) {
        assert(option != selector->options[i]);
    }
#endif // NDEBUG

    assert(SELECTOR_OPTIONS_MAX_COUNT > selector->count);
    selector->options[selector->count++] = (char)tolower(option);
}

NONNULL static void selector_clear(Selector* selector) {
    assert(NULL != selector);
    selector->count = 0;
}

#define SELECTOR_GET_SELECTION_BUFFER_SIZE 50
/**
 * Queries the user for a selection from the added options. Does not prompt.
 * Crashes if stdin could not be read.
 *
 * @return the user's selection, or a null terminator, if the option list is
 * empty.
 */
NONNULL static char selector_get_selection(const Selector *const selector) {
    assert(NULL != selector);

    if (0 == selector->count) return '\0';

    while (true) {
        static char buffer[SELECTOR_GET_SELECTION_BUFFER_SIZE] = {0};
        if (NULL == fgets(buffer, SELECTOR_GET_SELECTION_BUFFER_SIZE, stdin)) {
            handle_stdin_error();
        }

        char selection = '\0';
        for (const char* c = buffer; '\0' != *c; ++c)
            if (!isspace(*c)) {
                selection = *c;
                break;
            }
        if ('\0' == selection) continue;

        selection = (char)tolower(selection);
        bool valid_selection = false;
        for (size_t option = 0; option < selector->count; ++option)
            if (selection == selector->options[option]) {
                valid_selection = true;
                break;
            }
        if (!valid_selection) {
            printf("ERROR: invalid option '%c'!\n", selection);
            continue;
        }

        return selection;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Items and Inventory                                                        //
////////////////////////////////////////////////////////////////////////////////

// Zero-initalized.
typedef enum {
    ITEM_NONE = 0,
    ITEM_MEMORY_READ_ACCESS,
    ITEM_MEMORY_WRITE_ACCESS,
    ITEM_POINTER_DEREFERENCER,
    ITEM_OS_OVERRIDE_CAPABILITY,
    ITEM_RANSOMWARE_CODE_FRAGMENT,
    ITEM_VULNERABILITY,
    ITEM_SANDBOXER,
    // The RANSOMWARE is stored on the map as an item since there is not going
    // to be an item in that room anyways.
    ITEM_RANSOMWARE,
} ItemType;

static const char* item_type_name(const ItemType type) {
    switch (type) {
    case ITEM_MEMORY_READ_ACCESS:
        return "Full memory read access";
    case ITEM_MEMORY_WRITE_ACCESS:
        return "Full memory write access";
    case ITEM_POINTER_DEREFERENCER:
        return "Pointer dereferencer";
    case ITEM_OS_OVERRIDE_CAPABILITY:
        return "OS override capability";
    case ITEM_RANSOMWARE_CODE_FRAGMENT:
        return "RANSOMWARE code fragment";
    case ITEM_VULNERABILITY:
        return "Vulnerability";
    case ITEM_SANDBOXER:
        return "Sandboxer";
    case ITEM_NONE:
        return "None";
    case ITEM_RANSOMWARE:
        return "The RANSOMWARE";

    default:
        assert(false && "unreachable");
        exit(1);
    }
}

// Zero-initialized.
typedef struct {
    ItemType type;
    size_t   quantity;
} Item;

#define INVENTORY_MAX_COUNT 25
// Zero-initialized.
typedef struct {
    Item   items[INVENTORY_MAX_COUNT];
    size_t count;
} Inventory;

NONNULL static void inventory_add_item(
    Inventory *const inventory,
    const Item item
) {
    assert(NULL != inventory);

    if (ITEM_NONE == item.type) return;

    bool item_exists = false;
    for (size_t i = 0; i < inventory->count; ++i)
        if (item.type == inventory->items[i].type) {
            inventory->items[i].quantity += item.quantity;
            item_exists = true;
        }

    if (!item_exists) {
        assert(INVENTORY_MAX_COUNT > inventory->count);
        inventory->items[inventory->count++] = item;
    }
}

NONNULL static void inventory_try_remove_item(
    Inventory *const inventory,
    const ItemType   type,
    const size_t     quantity
) {
    assert(NULL != inventory);

    if (ITEM_NONE == type) return;

    for (size_t i = 0; i < inventory->count; ++i) {
        Item *const item = &inventory->items[i];
        if (type != item->type) continue;

        if (quantity >= item->quantity) {
            if (i != inventory->count - 1) {
                *item = inventory->items[inventory->count - 1];

            }
            --inventory->count;

        } else {
            item->quantity -= quantity;
        }
    }
}

NONNULL static void inventory_clear(Inventory *const inventory) {
    assert(NULL != inventory);
    inventory->count = 0;
}

NONNULL static size_t inventory_count_item(
    const Inventory *const inventory,
    const ItemType         type
) {
    assert(NULL != inventory);

    if (ITEM_NONE == type) return 0;

    for (size_t item = 0; item < inventory->count; ++item)
        if (type == inventory->items[item].type) {
            return inventory->items[item].quantity;
        }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Systems                                                                    //
////////////////////////////////////////////////////////////////////////////////

typedef enum {
    DIRECTION_UP = 0,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    // Quantifier.
    DIRECTION_COUNT,
} Direction;

static Direction direction_opposite(const Direction direction) {
    switch (direction) {
    case DIRECTION_UP:
        return DIRECTION_DOWN;
    case DIRECTION_DOWN:
        return DIRECTION_UP;
    case DIRECTION_LEFT:
        return DIRECTION_RIGHT;
    case DIRECTION_RIGHT:
        return DIRECTION_LEFT;

    case DIRECTION_COUNT:
    default:
        assert(false && "unreachable");
        exit(1);
    }
}

// Zero-initalized.
typedef enum {
    SCAN_NONE = 0,
    SCAN_EMPTY,
    SCAN_ABNORMAL,
    SCAN_SUSPICOUS,
    SCAN_ERROR,
} ScanResult;

static const char* scan_result_name(const ScanResult scan) {
    switch (scan) {
    case SCAN_NONE:
        return "";

    case SCAN_EMPTY:
        return "Empty";
    case SCAN_ABNORMAL:
        return "Abnormal";
    case SCAN_SUSPICOUS:
        return "Abnormal. Suspicous activity";
    case SCAN_ERROR:
        return "[ERROR]";

    default:
        assert(false && "unreachable");
        exit(1);
    }
}

typedef enum {
    SYSTEM_REGISTRY = 0,
    SYSTEM_NETWORK_INTERFACES,
    SYSTEM_KERNAL,
    SYSTEM_HARD_DRIVE,
    SYSTEM_WEBSURFER,
    SYSTEM_PAINTEREX,
    SYSTEM_BITMASHER,
    SYSTEM_ILO_LI_SINA_INTERPRETER,
    SYSTEM_FREEWRITER,
    SYSTEM_PIMG,
    SYSTEM_ESPRESSO_RUNTIME_ENVIROMENT,
    SYSTEM_SUPERCAD,
    SYSTEM_MACRODOI,
    SYSTEM_CONWAYS_IVORY_TOWER,
    SYSTEM_RANDOM_INFORMATION_GENERATOR,
    // Quantifier.
    SYSTEM_TYPE_COUNT,
    // Inital room. Must be after SYSTEM_TYPE_COUNT so map generator does not
    // try to generate an extra room for it.
    SYSTEM_BOOTLOADER,
} SystemType;

static const char* system_type_name(const SystemType type) {
    switch (type) {
    case SYSTEM_BOOTLOADER:
        return "The Bootloader";
    case SYSTEM_REGISTRY:
        return "The Registry";
    case SYSTEM_NETWORK_INTERFACES:
        return "The Network interfaces";
    case SYSTEM_KERNAL:
        return "The Kernal";
    case SYSTEM_HARD_DRIVE:
        return "The Hard drive";
    case SYSTEM_WEBSURFER:
        return "WebSurfer";
    case SYSTEM_PAINTEREX:
        return "PainterEX";
    case SYSTEM_BITMASHER:
        return "BitMasher";
    case SYSTEM_ILO_LI_SINA_INTERPRETER:
        return "The ilo li sina Interpreter";
    case SYSTEM_FREEWRITER:
        return "FreeWriter";
    case SYSTEM_PIMG:
        return "PIMG";
    case SYSTEM_ESPRESSO_RUNTIME_ENVIROMENT:
        return "The Espresso Runtime Enviroment";
    case SYSTEM_SUPERCAD:
        return "SuperCAD";
    case SYSTEM_MACRODOI:
        return "MacroDoi";
    case SYSTEM_CONWAYS_IVORY_TOWER:
        return "Conway's Ivory Tower";
    case SYSTEM_RANDOM_INFORMATION_GENERATOR:
        return "Random-Information-Generator";

    case SYSTEM_TYPE_COUNT:
    default:
        assert(false && "unreachable");
        exit(1);
    }
}

// Zero-initialized.
typedef struct System System;
struct System {
    SystemType type;
    ItemType   item;
    ScanResult scan_result;
    // Indexed by Direction. NULL means not present.
    System* adjacent[DIRECTION_COUNT];
};

/**
 * Attempts to perform a scan of the system. If successful, the sytem's scan
 * result will be updated.
 */
NONNULL static void system_try_scan(System *const system, const bool can_fail) {
    assert(NULL != system);

    if (can_fail && SCAN_FAIL_CHANCE > rand() % 100) {
        system->scan_result = SCAN_ERROR;
        return;
    }

    switch (system->item) {
    case ITEM_RANSOMWARE: {
        system->scan_result = SCAN_SUSPICOUS;
    }
    return;

    case ITEM_NONE: {
        system->scan_result = SCAN_EMPTY;
    }
    return;

    case ITEM_MEMORY_READ_ACCESS:
    case ITEM_MEMORY_WRITE_ACCESS:
    case ITEM_POINTER_DEREFERENCER:
    case ITEM_OS_OVERRIDE_CAPABILITY:
    case ITEM_RANSOMWARE_CODE_FRAGMENT:
    case ITEM_VULNERABILITY:
    case ITEM_SANDBOXER: {
        system->scan_result = SCAN_ABNORMAL;
    }
    return;

    default:
        assert(false && "unreachable");
    }
}

////////////////////////////////////////////////////////////////////////////////
// Map                                                                        //
////////////////////////////////////////////////////////////////////////////////

#define MAP_MAX_COUNT 50
// Zero-initialized.
// Must be heap allocated, else systems[n]->adjacent could be invalidated.
typedef struct {
    // First system is the root node of the map.
    System systems[MAP_MAX_COUNT];
    size_t count;
} Map;

/**
 * Allocates a new system from the map's internal buffer and returns it.
 *
 * You do not need to free this pointer.
 */
NONNULL static System* map_alloc_system(Map *const map) {
    assert(NULL != map);
    assert(MAP_MAX_COUNT > map->count);

    return &map->systems[map->count++];
}

/**
 * Attempts to place a system and item on the given map.
 *
 * @return true if successful.
 */
NONNULL static bool map_try_place_system(
    Map *const       map,
    const SystemType system,
    const ItemType   item
) {
    assert(NULL != map);
    assert(0 < map->count);

    const System* previous_system = NULL;
    System*       traverser       = &map->systems[0];

    // Traverse map and a system to it.
    for (size_t steps_left = MAX_STEPS; 0 < steps_left; --steps_left) {
        if (MOVE_CHANCE > rand() % 100) {
            // Gathers possible systems to move to.
            System* adjacents[DIRECTION_COUNT] = {0};
            size_t  adjacents_count            = 0;
            for (
                Direction direction = 0;
                direction < DIRECTION_COUNT;
                ++direction
            ) {
                System* adjacent = traverser->adjacent[direction];
                if (NULL != adjacent && adjacent != previous_system) {
                    adjacents[adjacents_count++] = adjacent;
                }
            }
            if (0 == adjacents_count) continue;

            previous_system = traverser;
            traverser       = adjacents[(size_t)rand() % adjacents_count];

        } else {
            // Gathers possible directions to add systems to.
            Direction directions[DIRECTION_COUNT] = {0};
            size_t    directions_count            = 0;
            for (
                Direction direction = 0;
                direction < DIRECTION_COUNT;
                ++direction
            ) {
                if (NULL == traverser->adjacent[direction]) {
                    directions[directions_count++] = direction;
                }
            }
            if (0 == directions_count) continue;

            System *const next_system = map_alloc_system(map);
            next_system->type         = system;
            next_system->item         = item;
            const Direction direction =
                directions[(size_t)rand() % directions_count];
            traverser->adjacent[direction] = next_system;
            next_system->adjacent[direction_opposite(direction)] = traverser;

            return true;
        }
    }

    return false;
}

/**
 * Generates a new map.
 * @param items - the items to place on the map. The items that couldn't be
 * placed will be removed from this inventory.
 * @return caller must free().
 */
NONNULL static Map* map_generate(Inventory *const items) {
    assert(NULL != items);

    Map *const map = calloc(1, sizeof(Map));
    if (NULL == map) {
        perror("Unable to allocate memory for map generation");
        exit(1);
    }
    // Map root node.
    System *const root_node = map_alloc_system(map);
    root_node->type         = SYSTEM_BOOTLOADER;
    root_node->item         = ITEM_NONE;

    const size_t expected_item_count = items->count;
    Inventory    item_pool           = *items;
    // Clear the source items inventory so we can add back only the ones that
    // were placed on the map.
    inventory_clear(items);

    SystemType system_pool[SYSTEM_TYPE_COUNT] = {0};
    for (SystemType type = 0; type < ARRAY_SIZE(system_pool); ++type) {
        system_pool[type] = type;
    }
    // Shuffle systems.
    // https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
    for (size_t i = 0; i < ARRAY_SIZE(system_pool); ++i) {
        size_t j = i + (size_t)rand() % (ARRAY_SIZE(system_pool) - i);
        SystemType type = system_pool[i];
        system_pool[i]  = system_pool[j];
        system_pool[j]  = type;
    }

    size_t systems_generated = 0;

    // All requried items must be generated, but not all rooms, thus we iterate
    // through each item and generate a room for it.
    size_t next_system_index = 0;
    while (0 < item_pool.count) {
        // If this index meets or exceeds the last index in the system pool,
        // then there are more items then systems, so we need to prioritize
        // placing the RANSOMWARE.
        if (next_system_index >= ARRAY_SIZE(system_pool) - 1) {
            break;
        }

        const bool placed_item =
            map_try_place_system(
                map,
                system_pool[next_system_index],
                item_pool.items[0].type
            );

        if (placed_item) {
            ++systems_generated;
            ++next_system_index;

            // Mark item as present.
            inventory_add_item(items, (Item) {
                .type     = item_pool.items[0].type,
                .quantity = 1
            });
        }

        // Consume item.
        inventory_try_remove_item(
            &item_pool
            , item_pool.items[0].type
            , 1
        );
    }

    // We place the RANSOMWARE last so so that there is a path to every item.
    if (!map_try_place_system(
                map
                , system_pool[next_system_index]
                , ITEM_RANSOMWARE
            )
       ) {
        fprintf(stderr, "ERROR: Unable to place ransomware on the map\n");
        exit(1);
    }
    ++systems_generated;

    // Warning for partial map generation.
    if (expected_item_count != items->count) {
        fprintf(stderr, "WARN: Unable to generate enough systems\n");
        fprintf(
            stderr,
            "      Could only place %zu items from a pool of %zu\n"
            , items->count, expected_item_count
        );
        fprintf(
            stderr,
            "      There are only %zu systems avalible in total for "
            "generation\n",
            systems_generated
        );
    }

    return map;
}

////////////////////////////////////////////////////////////////////////////////
// Lose Sequence                                                              //
////////////////////////////////////////////////////////////////////////////////

/**
 * Returns a random printable ASCII character.
 */
static char random_character(void) {
    return 0x21 + (char)(rand() % (0x7E - 0x21));
}

/**
 * Randomly replaces characters in the given string.
 */
NONNULL static void garble_string(char *const string) {
    assert(NULL != string);
    for (char* c = string; '\0' != *c; ++c) {
        if (0 == rand() % 5) *c = random_character();
    }
}

/**
 * Randomly upper/lower cases the characters in the given string.
 */
NONNULL static void annoying_case_string(char *const string) {
    assert(NULL != string);
    for (char* c = string; '\0' != *c; ++c) {
        if (0 == rand() % 2) {
            *c = (char)toupper(*c);
        } else {
            *c = (char)tolower(*c);
        }
    }
}

/**
 * Creates a mutable copy of the given string.
 * @return caller must free().
 */
NONNULL static char* string_copy_mutable(const char *const string) {
    assert(NULL != string);

    const size_t length = strlen(string);
    char*        copy   = calloc(1 + length, 1);

    if (NULL == copy) {
        perror("ERROR: Failed to allocate memory for string copy\n");
        exit(1);
    }

    strcpy(copy, string);

    return copy;
}

static void run_lose_sequence(const bool funny) {
    clear();

    if (funny) {
        for (size_t j = 0; j < 10; ++j) {
            for (size_t i = 0; i < 500; ++i) fputs(";)", stdout);
            fflush(stdout);
            sleep_ns(100000000);
        }
    } else {
        for (size_t j = 0; j < 10; ++j) {
            for (size_t i = 0; i < 1000; ++i) putchar(random_character());
            fflush(stdout);
            sleep_ns(100000000);
        }
    }

    clear();

    static const char *const text_1 = "GAME OVER GAME OVER GAME OVER";
    char* text_copy = string_copy_mutable(text_1); // Must free().
    for (size_t i = 0; i < 6; ++i) {
        delayed_print(true, "%s", text_copy);
        garble_string(text_copy);
    }
    delayed_print(true, "%s", text_copy);
    free(text_copy);
    text_copy = NULL;

    delayed_print_newline();

    static const char *const text_2 = "All Your systems are belong to us";
    text_copy = string_copy_mutable(text_2); // must free().
    annoying_case_string(text_copy);
    delayed_print(true, "%s", text_copy);
    free(text_copy);
    text_copy = NULL;

    delayed_print_newline();

    for (size_t i = 0; i < 3; ++i) {
        delayed_print(true, ";;;;;;;;)))))");
    }

    delayed_print_newline();

    await_player(true);
}

////////////////////////////////////////////////////////////////////////////////
// Battle                                                                     //
////////////////////////////////////////////////////////////////////////////////

// Represents a fighter in a battle, complete with health, damage, and digital
// bloodlust
typedef struct {
    const char* name;
    int         health;
    int         damage;
} Fighter;

// printf format for Fighter.
#define FIGHTER_FMT "%s: %d hp, %d dmg"
// printf argument destructurizer for Fighter. Pass by pointer.
#define FIGHTER_ARG(fighter) (fighter)->name, (fighter)->health, (fighter)->damage

NONNULL static int fighter_attack(
    const Fighter *const attacker,
    Fighter *const       victim
) {
    assert(NULL != attacker);
    assert(NULL != victim);

    victim->health -= attacker->damage;
    return attacker->damage;
}

static void run_boss_battle_intro(void) {
    clear();

    delayed_print(true, "The RANSOMWARE");
    delayed_print_newline();
    delayed_print(
        true,
        "You have located the RANSOMWARE infecting the computer"
    );
    delayed_print(true, "EXTRACT it from the system as soon as possible");
    delayed_print(true, "There is no other option");
    delayed_print_newline();

    await_player(true);
}

/**
 * @param remaining_items - the items that the player did not obtain.
 * @param lose_time - the time that the player loses if equal to get_time_s().
 */
NONNULL static void run_boss_battle(
    const Inventory *const remaining_items,
    const long lose_time
) {
    assert(NULL != remaining_items);

    run_boss_battle_intro();

    const bool can_alter_memory =
    0 == inventory_count_item(remaining_items, ITEM_MEMORY_READ_ACCESS)
    && 0 == inventory_count_item(remaining_items, ITEM_MEMORY_WRITE_ACCESS);
    const bool has_admin_privileges =
    0 == inventory_count_item(remaining_items, ITEM_OS_OVERRIDE_CAPABILITY);
    const bool has_dereferencer =
    0 == inventory_count_item(remaining_items, ITEM_POINTER_DEREFERENCER);
    const bool has_sandboxer =
    0 == inventory_count_item(remaining_items, ITEM_SANDBOXER);

    Fighter player = (Fighter) {
        .name   = "You",
        .health = FIGHTER_BASE_HEALTH,
        .damage = FIGHTER_BASE_DAMAGE + PLAYER_DAMAGE_BOOST,
    };
    const size_t remaining_code_fragments =
        inventory_count_item(remaining_items, ITEM_RANSOMWARE_CODE_FRAGMENT);
    const size_t remaining_vulernabilities =
        inventory_count_item(remaining_items, ITEM_VULNERABILITY);
    Fighter ransomware = (Fighter) {
        .name   = "The RANSOMWARE",
        .health = FIGHTER_BASE_HEALTH + CODE_FRAGMENT_HEALTH_BOOST * (int)remaining_code_fragments,
        .damage = FIGHTER_BASE_DAMAGE + VULNERABILITY_DAMAGE_BOOST * (int)remaining_vulernabilities
    };

    Selector fight_menu = {0};
    selector_add_option(&fight_menu, 'x'); // E(X)TRACT.
    selector_add_option(&fight_menu, 'n'); // Do (N)OTHING.
    selector_add_option(&fight_menu, 'd'); // Do a funny (D)ANCE.
    selector_add_option(&fight_menu, 'e'); // (E)XIT game.

    while (true) {
        const long current_time = get_time_s();
        if (!has_sandboxer && current_time >= lose_time) {
            run_lose_sequence(false);
            return;
        }

        clear();
        delayed_print(true, "The RANSOMEWARE");
        if (!has_sandboxer) {
            delayed_print(
                true,
                "Time left: %ld second(s)",
                lose_time - current_time
            );
        }
        delayed_print_newline();
        delayed_print(false, FIGHTER_FMT, FIGHTER_ARG(&player));
        delayed_print(false, FIGHTER_FMT, FIGHTER_ARG(&ransomware));
        delayed_print_newline();
        delayed_print(false, "E(X)TRACT");
        delayed_print(false, "Do (N)OTHING");
        delayed_print(false, "Do a funny (D)ANCE");
        delayed_print(false, "(E)XIT game");

        const char choice = selector_get_selection(&fight_menu);
        switch (choice) {
        case 'x': {
            sleep_ns(BATTLE_MOVE_DELAY);
            delayed_print_newline();
            delayed_print(false, "You attempt to EXTRACT the RANSOMWARE...");
            sleep_ns(BATTLE_MOVE_DELAY);

            if (!has_dereferencer) {
                delayed_print(
                    false,
                    "Unable to locate relavent memory to alter; you lack the "
                    "capabilities"
                );
            } else if (!can_alter_memory) {
                delayed_print(
                    false,
                    "Unable to alter relavent memory; you lack the capabilities"
                );
            } else if (!has_admin_privileges) {
                delayed_print(
                    false,
                    "Memory alteration denied; you lack sufficent privileges"
                );
            } else {
                const int damage = fighter_attack(&player, &ransomware);
                delayed_print(
                    false,
                    "You complete partial code EXTRACTion, dealing %d dmg (%d "
                    "hp remaining)",
                    damage, ransomware.health
                );

                if (0 >= ransomware.health) {
                    sleep_ns(BATTLE_MOVE_DELAY);
                    clear();
                    delayed_print(true, "Congratulations");
                    delayed_print_newline();
                    delayed_print(
                        true,
                        "You have successfully EXTRACTed the RANSOMWARE"
                    );
                    delayed_print_newline();
                    await_player(true);
                    return;
                }
            }
        }
        break;

        case 'n': {
            sleep_ns(BATTLE_MOVE_DELAY);
            delayed_print_newline();
            delayed_print(false, "You do absolutely NOTHING...");
        }
        break;

        case 'd': {
            sleep_ns(BATTLE_MOVE_DELAY);
            delayed_print_newline();
            delayed_print(false, "You attempt a funny DANCE...");
            sleep_ns(BATTLE_MOVE_DELAY);
            delayed_print(
                false,
                "You are an antivirus, you have no means to DANCE"
            );
            sleep_ns(BATTLE_MOVE_DELAY);
            const int damage = fighter_attack(&player, &player);
            delayed_print(
                false,
                "In the process you corrupted your own data, dealing %d dmg "
                "(%d hp remaining)",
                damage, player.health
            );
            sleep_ns(BATTLE_MOVE_DELAY);

            if (0 >= player.health) {
                sleep_ns(BATTLE_MOVE_DELAY);
                run_lose_sequence(true);
                return;
            }
        }
        break;

        case 'e':
            return;

        default:
            assert(false && "unreachable");
        }

        // RANSOMWARE attack sequence.
        sleep_ns(BATTLE_MOVE_DELAY);
        delayed_print_newline();
        delayed_print(false, "The RANSOMWARE attempts to deliver a payload...");
        sleep_ns(BATTLE_MOVE_DELAY);
        const int damage = fighter_attack(&ransomware, &player);
        delayed_print(
            false,
            "You were hit with a viral payload, dealing %d dmg (%d hp "
            "remaining)",
            damage, player.health
        );

        if (0 >= player.health) {
            sleep_ns(BATTLE_MOVE_DELAY);
            run_lose_sequence(false);
            return;
        }

        sleep_ns(BATTLE_MOVE_DELAY);
        await_player(false);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Game                                                                       //
////////////////////////////////////////////////////////////////////////////////

/**
 * Fills the given inventory with the items the player needs to find to win the
 * game.
 */
NONNULL static void generate_required_items(Inventory *const inventory) {
    assert(NULL != inventory);

    inventory_add_item(inventory, (Item) {
        .type     = ITEM_MEMORY_READ_ACCESS,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type     = ITEM_MEMORY_WRITE_ACCESS,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type     = ITEM_POINTER_DEREFERENCER,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type     = ITEM_OS_OVERRIDE_CAPABILITY,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type     = ITEM_SANDBOXER,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type     = ITEM_RANSOMWARE_CODE_FRAGMENT,
        .quantity = 1 + (size_t)rand() % 3
    });
    inventory_add_item(inventory, (Item) {
        .type     = ITEM_VULNERABILITY,
        .quantity = 1 + (size_t)rand() % 3
    });

    // Shuffle items.
    // https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
    for (size_t i = 0; i < inventory->count; ++i) {
        const size_t j      = i + (size_t)rand() % (inventory->count - i);
        const Item item     = inventory->items[i];
        inventory->items[i] = inventory->items[j];
        inventory->items[j] = item;
    }
}

NONNULL static void display_inventory(
    const Inventory *const inventory,
    const Inventory *const required_items
) {
    assert(NULL != inventory);
    assert(NULL != required_items);

    clear();
    delayed_print(true, "INVENTORY:");
    delayed_print_newline();
    if (0 == inventory->count) {
        delayed_print(true, "Empty...");
    } else {
        for (size_t i = 0; i < inventory->count; ++i) {
            const Item *const item = &inventory->items[i];
            delayed_print(
                true,
                "- %s: %zu",
                item_type_name(item->type), item->quantity
            );
        }
    }

    delayed_print_newline();
    delayed_print(true, "Remaining Items:");
    delayed_print_newline();
    if (0 == required_items->count) {
        delayed_print(true, "Everything needed has been found...");
    } else {
        for (size_t i = 0; i < required_items->count; ++i) {
            const Item *const item = &required_items->items[i];
            delayed_print(
                true,
                "- %s: %zu",
                item_type_name(item->type), item->quantity
            );
        }
    }

    delayed_print_newline();
    await_player(true);
}

static void run_game(void) {
    Inventory required_items = {0};
    generate_required_items(&required_items);
    Map* map = map_generate(&required_items); // Must free().

    const long lose_time = get_time_s() + (long)required_items.count *
                           SECONDS_PER_SYSTEM;

    Inventory inventory = {0};
    Selector  game_menu = {0};
    assert(0 < map->count);
    System* current_system = &map->systems[0];

    bool game_running = true;
    while (game_running) {
        const long current_time = get_time_s();
        if (current_time >= lose_time) {
            run_lose_sequence(false);
            goto lend_game;
        }

        if (ITEM_RANSOMWARE == current_system->item) {
            run_boss_battle(&required_items, lose_time);
            goto lend_game;
        }

        // Scan the current system if the user moved into an unscanned room.
        system_try_scan(current_system, false);

        // Display status.
        clear();
        if (SCAN_NONE != current_system->scan_result) {
            delayed_print(
                true,
                "%s (scan: %s)"
                , system_type_name(current_system->type)
                , scan_result_name(current_system->scan_result)
            );
        } else {
            delayed_print(true, "%s", system_type_name(current_system->type));
        }
        delayed_print(
            true,
            "Time left: %ld second(s)"
            , lose_time - current_time
        );
        delayed_print_newline();

        selector_clear(&game_menu);

        // Adds possible directions to move in.
        for (Direction direction = 0; direction < DIRECTION_COUNT; ++direction) {
            const System *const system = current_system->adjacent[direction];
            if (NULL == system) continue;

            switch (direction) {
            case DIRECTION_UP: {
                if (SCAN_NONE != system->scan_result) {
                    delayed_print(
                        false,
                        "[%s (scan: %s)] is (U)P above"
                        , system_type_name(system->type)
                        , scan_result_name(system->scan_result)
                    );
                } else {
                    delayed_print(
                        false,
                        "[%s] is (U)P above"
                        , system_type_name(system->type)
                    );
                }
                selector_add_option(&game_menu, 'u');
            }
            break;
            case DIRECTION_DOWN: {
                if (SCAN_NONE != system->scan_result) {
                    delayed_print(
                        false,
                        "[%s (scan: %s)] is (D)OWN below"
                        , system_type_name(system->type)
                        , scan_result_name(system->scan_result)
                    );
                } else {
                    delayed_print(
                        false,
                        "[%s] is (D)OWN below"
                        , system_type_name(system->type)
                    );
                }
                selector_add_option(&game_menu, 'd');
            }
            break;
            case DIRECTION_LEFT: {
                if (SCAN_NONE != system->scan_result) {
                    delayed_print(
                        false,
                        "[%s (scan: %s)] is to the (L)EFT"
                        , system_type_name(system->type)
                        , scan_result_name(system->scan_result)
                    );
                } else {
                    delayed_print(
                        false,
                        "[%s] is to the (L)EFT"
                        , system_type_name(system->type)
                    );
                }
                selector_add_option(&game_menu, 'l');
            }
            break;
            case DIRECTION_RIGHT: {
                if (SCAN_NONE != system->scan_result) {
                    delayed_print(
                        false,
                        "[%s (scan: %s)] is to the (R)IGHT"
                        , system_type_name(system->type)
                        , scan_result_name(system->scan_result)
                    );
                } else {
                    delayed_print(
                        false,
                        "[%s] is to the (R)IGHT"
                        , system_type_name(system->type)
                    );
                }
                selector_add_option(&game_menu, 'r');
            }
            break;

            case DIRECTION_COUNT:
            default:
                assert(false && "unreachable");
            }
        }

        // Adds ability to take room's item.
        if (ITEM_NONE != current_system->item) {
            delayed_print(
                false,
                "There is a [%s]. (T)AKE it?"
                , item_type_name(current_system->item)
            );
            selector_add_option(&game_menu, 't');
        }

        // Adds standard actions.
        delayed_print_newline();
        delayed_print(false, "(S)CAN the neighboring systems");
        selector_add_option(&game_menu, 's');
        delayed_print(false, "Open the (I)NVENTORY");
        selector_add_option(&game_menu, 'i');
        delayed_print(false, "(E)XIT game");
        selector_add_option(&game_menu, 'e');

        const char choice = selector_get_selection(&game_menu);
        switch (choice) {
        case 'u': {
            current_system = current_system->adjacent[DIRECTION_UP];
        }
        break;
        case 'd': {
            current_system = current_system->adjacent[DIRECTION_DOWN];
        }
        break;
        case 'l': {
            current_system = current_system->adjacent[DIRECTION_LEFT];
        }
        break;
        case 'r': {
            current_system = current_system->adjacent[DIRECTION_RIGHT];
        }
        break;

        case 't': {
            inventory_add_item(&inventory, (Item) {
                .type     = current_system->item,
                .quantity = 1,
            });
            inventory_try_remove_item(&required_items, current_system->item, 1);
            current_system->item = ITEM_NONE;
        }
        break;

        case 's': {
            delayed_print_newline();
            delayed_print(false, "SCANning...");
            sleep_ns(SCAN_TIME_NS);

            for (
                Direction direction = 0;
                direction < DIRECTION_COUNT;
                ++direction
            ) {
                System *const adjacent = current_system->adjacent[direction];
                if (NULL != adjacent) system_try_scan(adjacent, true);
            }
        }
        break;

        case 'i':
            display_inventory(&inventory, &required_items);
            break;

        case 'e':
            goto lend_game;

        default:
            assert(false && "unreachable");
        }
    }
lend_game:

    free(map);
    map = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Main Menu                                                                  //
////////////////////////////////////////////////////////////////////////////////

#ifdef PLATFORM_DOS
#  define PLATFORM_STRING "DOS"
#elif defined(PLATFORM_POSIX) // PLATFORM_DOS
#  define PLATFORM_STRING "POSIX"
#else // PLATFORM_POSIX
#  error Unsupported platform
#endif

static const char *const logo[] = {
    " ______  __________________ _______  _______  _______           _______  _______ ",
    "(  ___ \\ \\__   __/\\__   __/(       )(  ___  )(  ____ \\|\\     /|(  ____ \\(  ____ )",
    "| (   ) )   ) (      ) (   | () () || (   ) || (    \\/| )   ( || (    \\/| (    )|",
    "| (__/ /    | |      | |   | || || || (___) || (_____ | (___) || (__    | (____)|",
    "|  __ (     | |      | |   | |(_)| ||  ___  |(_____  )|  ___  ||  __)   |     __)",
    "| (  \\ \\    | |      | |   | |   | || (   ) |      ) || (   ) || (      | (\\ (   ",
    "| )___) )___) (___   | |   | )   ( || )   ( |/\\____) || )   ( || (____/\\| ) \\ \\__",
    "|/ \\___/ \\_______/   )_(   |/     \\||/     \\|\\_______)|/     \\|(_______/|/   \\__/"
};

static const char *const version = "V7.6496437096";

static void run_instructions_menu(void) {
    clear();

    delayed_print(true, "INSTRUCTIONS");
    delayed_print_newline();
    delayed_print(
        true,
        "You are an antivirus trying to rid a computer of a RANSOMWARE before "
        "it takes over the system. There is a finite amount of time before the "
        "system is fully infected"
    );
    delayed_print_newline();
    delayed_print(
        true,
        "In order to defeat it, you must find all items before you find the "
        "RANSOMWARE. If you do not, you will not be able to EXTRACT it and you "
        "will lose."
    );
    delayed_print_newline();
    delayed_print(
        true,
        "Each system (room) contains an item, which you can move to; UP, DOWN, "
        "LEFT, AND RIGHT. Keep in mind that the map is NOT 2D; Moving RIGHT, "
        "UP, LEFT, and DOWN will lead to a different room than the one you "
        "started in. The map is 'Spiky' so-to-speak."
    );
    delayed_print_newline();
    delayed_print(
        true,
        "You have a SCANner to aid in figuring out which rooms contain items "
        "and which have RANSOMWARE. Using the SCANner will reveal what the "
        "surrounding rooms contain, and the room you are currently in will be "
        "automatically SCANned for you. But beware: SCANning takes time. Also, "
        "occasionaly a SCAN will fail and need to be repeated."
    );
    delayed_print_newline();
    delayed_print(true, "Good luck");
    delayed_print_newline();

    await_player(true);
}

static void run_license_menu(void) {
    clear();

    delayed_print(true, "LICENSE");
    delayed_print_newline();
    delayed_print(
        true,
        "Copyright (C) 2024-2025 ona-li-toki-e-jan-Epiphany-tawa-mi."
    );
    delayed_print_newline();
    delayed_print(
        true,
        "This program is free software: you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation, either version 3 of the License, or (at "
        "your option) any later version."
    );
    delayed_print_newline();
    delayed_print(
        true,
        "This program is distributed in the hope that it will be useful, but "
        "WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
        "General Public License for more details."
    );
    delayed_print_newline();
    delayed_print(
        true,
        "You should have received a copy of the GNU General Public License "
        "along with this program. If not, see http://www.gnu.org/licenses/."
    );
    delayed_print_newline();
    delayed_print(
        true,
        "Source (paltepuk):"
    );
    delayed_print(
        true,
        "Clearnet - https://paltepuk.xyz/cgit/BitMasher.git/about/"
    );
    delayed_print(
        true,
        "I2P - http://oytjumugnwsf4g72vemtamo72vfvgmp4lfsf6wmggcvba3qmcsta.b32.i2p/cgit/BitMasher.git/about/"
    );
    delayed_print(
        true,
        "Tor - http://4blcq4arxhbkc77tfrtmy4pptf55gjbhlj32rbfyskl672v2plsmjcyd.onion/cgit/BitMasher.git/about/"
    );
    delayed_print_newline();
    delayed_print(
        true,
        "Source (GitHub):"
    );
    delayed_print(
        true,
        "Clearnet - https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/BitMasher/"
    );
    delayed_print_newline();

    await_player(true);
}

static void run_exit_sequence(void) {
    printf("EXITing");
    fflush(stdout);
    sleep_ns(DELAYED_PRINT_DELAY_NS);
    printf(".");
    fflush(stdout);
    sleep_ns(DELAYED_PRINT_DELAY_NS);
    printf(".");
    fflush(stdout);
    sleep_ns(DELAYED_PRINT_DELAY_NS);
    printf(".\n");
}

/**
 * @return true if the user chose to exit the game.
 */
static bool run_start_menu(void) {
    Selector start_menu = {0};
    selector_add_option(&start_menu, 'p'); // (P)LAY.
    selector_add_option(&start_menu, 'i'); // (I)NSTRUCTIONS.
    selector_add_option(&start_menu, 'l'); // (L)ICENSE.
    selector_add_option(&start_menu, 'e'); // (E)XIT.

    while (true) {
        clear();

        for (size_t line = 0; line < ARRAY_SIZE(logo); ++line) {
            delayed_print(true, "%s", logo[line]);
        }
        delayed_print_newline();
        delayed_print(true, "%s ("PLATFORM_STRING")", version);
        delayed_print_newline();
        delayed_print(
            true,
            "Type and enter the character in paranthesis to select an option."
        );
        delayed_print_newline();
        delayed_print(true, "(P)LAY");
        delayed_print(true, "(I)NSTRUCTIONS");
        delayed_print(true, "(L)ICENSE");
        delayed_print(true, "(E)XIT");

        const char choice = selector_get_selection(&start_menu);
        switch (choice) {
        case 'p': {
            return false;
        }
        case 'i': {
            run_instructions_menu();
            break;
        }
        case 'l': {
            run_license_menu();
            break;
        }
        case 'e': {
            run_exit_sequence();
            return true;
        }
        default: {
            assert(false && "unreachable");
        }
        }
    }
}

static void on_exit(void) {
    // Restores terminal state.
    // \x1B[?1049l - Disable alternative buffer.
    // \x1B[?47l   - Restore screen.
    // \x1B[u      - Restore cursor position.
    printf("\x1B[?1049l\x1B[?47l\x1B[u");
}

int main(void) {
#ifdef PLATFORM_POSIX
    if (!isatty(STDOUT_FILENO)) {
        fprintf(stderr, "ERROR: stdout is not a terminal\n");
        return 1;
    }
#endif // PLATFORM_POSIX

    // Seed random number generator.
    srand((unsigned int)get_time_s());

    // Saves terminal state.
    // \x1B[s      - Save cursor position.
    // \x1B[?47h   - Save screen.
    // \x1B[?1049h - Enable alternative buffer.
    printf("\x1B[s\x1B[?47h\x1B[?1049h");

    const bool registered_on_exit = 0 == atexit(&on_exit);

    while (true) {
        if (run_start_menu()) break;
        run_game();
    }

    if (!registered_on_exit) on_exit();

    return 0;
}
