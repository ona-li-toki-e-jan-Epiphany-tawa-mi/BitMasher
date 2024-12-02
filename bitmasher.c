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
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// POSIX.
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////
// Configuration                                                              //
////////////////////////////////////////////////////////////////////////////////

// The amount of time the player is given per system generated.
#define SECONDS_PER_SYSTEM 8

// The time each line printed with delayed_print() waits for in nanoseconds.
#define DELAYED_PRINT_DELAY_NS 110000000L

// The number of steps the traverser can take before it gives up. Higher values
// means it's more likely to generate a room, but loading times will have the
// potential to increase.
#define MAX_STEPS 100
// The chance that the traverser choses to move to an existing room over finding
// a new one, given as a number between 0 and 1. Make larger for spikier maps.
#define MOVE_CHANCE 70

////////////////////////////////////////////////////////////////////////////////
// Utilities                                                                  //
////////////////////////////////////////////////////////////////////////////////

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

/*
 * Macro to typecheck printf-like functions.
 * Works in gcc and clang.
 * format - the argument number (1-indexed) that has the format string.
 * va_list - the argument number that has the variable argument list (...).
 */
#ifdef __GNUC__
#  define PRINTF_TYPECHECK(format, va_list) \
    __attribute__ ((__format__ (printf, format, va_list)))
#else // __GNUC__
#  define PRINTF_TYPECHECK(format, va_list)
#endif

// Zero-initialized.
typedef struct {
    unsigned long width;
    unsigned long height;
} Terminal;

static const Terminal* terminal_size(void) {
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

/**
 * Sleeps for the specified number of nanoseconds.
 * @param nanoseconds - must be 0 <= nanoseconds < 1,000,000,000.
 */
static void sleep_ns(long nanoseconds) {
    assert(1000000000 > nanoseconds);
    struct timespec time = { .tv_sec = 0, .tv_nsec = nanoseconds };
    (void)nanosleep(&time, NULL);
}

static long get_time_s(void) {
    struct timespec tp;
    if (-1 == clock_gettime(CLOCK_MONOTONIC, &tp)) {
        perror("Failed to read from monotonic clock");
        exit(1);
    }

    return tp.tv_sec;
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
                                  , size_t length
                                  ) {
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

#define DELAYED_PRINT_MAX_BUFFER_SIZE 512
/**
 * Prints a message line-by-line. Each line is printed with a delay to give an
 * old computer vibe.
 *
 * @param center - whether to print the lines centered in the terminal.
 * @param format - like with the printf function family.
 */
static void PRINTF_TYPECHECK(2, 3) delayed_print( bool center
                                                 , const char* format
                                                 , ...
                                                 ) {
    assert(NULL != format);

    // Use vsnprintf to handle the format string.
    static char buffer[DELAYED_PRINT_MAX_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, DELAYED_PRINT_MAX_BUFFER_SIZE, format, args);
    if (0 > result) {
        (void)fprintf(stderr, "ERROR: "__FILE__":%s: failed to run vsnprintf: "
                              "%s\n", __func__, strerror(errno));
        exit(1);
    } else if (result >= DELAYED_PRINT_MAX_BUFFER_SIZE) {
        (void)fprintf(stderr, "WARN: "__FILE__":%s: output truncated when "
                              "running vsnprintf with format '%s'\n", __func__
                            , format);
    }
    va_end(args);

    const Terminal* terminal = terminal_size();
    const char*     start    = buffer;
    const char*     end      = start;
    size_t          length   = 0;

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
static void delayed_print_newline(void) {
    delayed_print(false, "\n");
}

static void clear(void) {
    // \x1B[2J - clears screen.
    // \x1B[H  - returns cursor to home position.
    (void)fputs("\x1B[2J\x1B[H", stdout);
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

#define SELECTOR_OPTIONS_MAX_COUNT  25
// Zero-initialized.
typedef struct {
    char   options[SELECTOR_OPTIONS_MAX_COUNT];
    size_t count;
} Selector;

static void selector_add_option(Selector* selector, char option) {
    assert(NULL != selector);
    assert(!isspace(option));

    // TODO assert that option does not already exist.

    assert(SELECTOR_OPTIONS_MAX_COUNT > selector->count);
    selector->options[selector->count++] = (char)tolower(option);
}

static void selector_clear(Selector* selector) {
    assert(NULL != selector);

    selector->count = 0;
}

#define SELECTOR_GET_SELECTION_BUFFER_SIZE 50
static char selector_get_selection(const Selector* selector) {
    assert(NULL != selector);

    if (0 == selector->count) return '\0';

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
        for (size_t option = 0; option < selector->count; ++option)
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
    ITEM_NONE = 0,
    ITEM_FULL_MEMORY_READ_ACCESS,
    ITEM_FULL_MEMORY_WRITE_ACCESS,
    ITEM_POINTER_DEREFERENCER,
    ITEM_OS_OVERRIDE_CAPABILITY,
    ITEM_RANSOMWARE_CODE_FRAGMENT,
    ITEM_VULNERABILITY,
    ITEM_SANDBOXER,
    // The RANSOMWARE is stored on the map as an item since there is not going
    // to be an item in that room anyways.
    ITEM_RANSOMWARE,
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

    default: assert(false && "unreachable");
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

static void inventory_add_item(Inventory* inventory, Item item) {
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

static void inventory_try_remove_item( Inventory* inventory
                                     , ItemType type
                                     , size_t quantity
                                     ) {
    assert(NULL != inventory);

    if (ITEM_NONE == type) return;

    for (size_t i = 0; i < inventory->count; ++i) {
        Item* item = &inventory->items[i];
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

static void inventory_clear(Inventory* inventory) {
    assert(NULL != inventory);

    inventory->count = 0;
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

static Direction direction_opposite(Direction direction) {
    switch (direction) {
    case DIRECTION_UP:    return DIRECTION_DOWN;
    case DIRECTION_DOWN:  return DIRECTION_UP;
    case DIRECTION_LEFT:  return DIRECTION_RIGHT;
    case DIRECTION_RIGHT: return DIRECTION_LEFT;

    case DIRECTION_COUNT:
    default: assert(false && "unreachable");
    }

    exit(1);
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

static const char* system_type_name(SystemType type) {
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
    default: assert(false && "unreachable");
    }

    exit(1);
}

// Zero-initialized.
typedef struct System System;
struct System {
    SystemType type;
    ItemType   item;
    // Indexed by Direction. NULL means not present.
    System* adjacent[DIRECTION_COUNT];
};

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

static System* map_alloc_system(Map* map) {
    assert(NULL != map);
    assert(MAP_MAX_COUNT > map->count);

    return &map->systems[map->count++];
}

static Map* generate_map(Inventory* items) {
    assert(NULL != items);

    Map* map = calloc(1, sizeof(Map));
    if (NULL == map) {
        perror("Unable to allocate memory for map generation");
        exit(1);
    }

    // Map root node.
    System* root_node = map_alloc_system(map);
    root_node->type   = SYSTEM_BOOTLOADER;
    root_node->item   = ITEM_NONE;

    size_t    expected_item_count = items->count;
    Inventory item_pool           = *items;
    // Clear the source items inventory so we can add back only the ones that
    // were placed on the map.
    inventory_clear(items);
    // We append the RANSOMWARE at the end so it is generated last so that there
    // is a path to every other item.
    inventory_add_item(&item_pool, (Item) {
        .type     = ITEM_RANSOMWARE,
        .quantity = 1
    });

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

    // Statistics.
    size_t steps_taken       = 0;
    size_t systems_generated = 0;

    // All requried items must be generated, but not all rooms, thus we iterate
    // through each item and generate a room for it.
    size_t next_system_index = 0;
    while (0 < item_pool.count) {
        // If this index meets or exceeds the last index in the system pool,
        // then there are more items then systems, so we need to prioritize
        // placing the RANSOMWARE.
        if (next_system_index >= ARRAY_SIZE(system_pool) - 1)
            while (1 < item_pool.count)
                for (size_t i = 0; i < item_pool.count; ++i) {
                    Item item = item_pool.items[i];
                    if (ITEM_RANSOMWARE != item.type) {
                        inventory_try_remove_item( &item_pool
                                                 , item.type
                                                 , item.quantity);
                        break;
                    }
                }

        bool    placed_item     = false;
        System* previous_system = NULL;
        System* traverser       = root_node;

        // Traverse map and a system to it.
        for (size_t steps_left = MAX_STEPS; 0 < steps_left; --steps_left) {
            ++steps_taken;

            if (MOVE_CHANCE > rand() % 100) {
                // Gathers possible systems to move to.
                System* adjacents[DIRECTION_COUNT] = {0};
                size_t  adjacents_count            = 0;
                for ( Direction direction = 0
                    ; direction < DIRECTION_COUNT
                    ; ++direction
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
                for ( Direction direction = 0
                    ; direction < DIRECTION_COUNT
                    ; ++direction
                    ) {
                    if (NULL == traverser->adjacent[direction]) {
                        directions[directions_count++] = direction;
                    }
                }
                if (0 == directions_count) continue;

                System* next_system = map_alloc_system(map);
                next_system->type   = system_pool[next_system_index];
                next_system->item   = item_pool.items[0].type;

                Direction direction
                    = directions[(size_t)rand() % directions_count];
                traverser->adjacent[direction] = next_system;
                next_system->adjacent[direction_opposite(direction)]
                    = traverser;
                placed_item = true;

                ++systems_generated;
                break;
            }
        }

        // Mark item as present.
        if (ITEM_RANSOMWARE != item_pool.items[0].type) {
            inventory_add_item(items, (Item) {
                .type     = item_pool.items[0].type,
                .quantity = 1
            });
        }
        // Consume item.
        inventory_try_remove_item( &item_pool
                                 , item_pool.items[0].type
                                 , 1);
        // If we couldn't place the item, we still might be able to use the
        // system for the next one.
        if (placed_item) ++next_system_index;
    }

    // Warning for partial map generation.
    if (expected_item_count != items->count) {
        clear();
        (void)fprintf(stderr, "WARN: Unable to generate enough systems\n");
        (void)fprintf(stderr, "      Could only place %zu items from a pool of "
                              "%zu\n", items->count, expected_item_count);
        (void)fprintf(stderr, "      There are only %zu systems avalible in "
                              "total for generation\n", systems_generated);
    }

    (void)fprintf(stderr, "INFO: Map generator statistics:\n");
    (void)fprintf(stderr, "      Total traverser steps - %zu\n", steps_taken);
    (void)fprintf(stderr, "      Systems generated - %zu\n", systems_generated);

    return map;
}

////////////////////////////////////////////////////////////////////////////////
// Battle                                                                     //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Game                                                                       //
////////////////////////////////////////////////////////////////////////////////

static void generate_required_items(Inventory* inventory) {
    assert(NULL != inventory);

    inventory_add_item(inventory, (Item) {
        .type = ITEM_FULL_MEMORY_READ_ACCESS,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type = ITEM_FULL_MEMORY_WRITE_ACCESS,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type = ITEM_POINTER_DEREFERENCER,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type = ITEM_OS_OVERRIDE_CAPABILITY,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type = ITEM_SANDBOXER,
        .quantity = 1
    });
    inventory_add_item(inventory, (Item) {
        .type = ITEM_RANSOMWARE_CODE_FRAGMENT,
        .quantity = 1 + (size_t)rand() % 3
    });
    inventory_add_item(inventory, (Item) {
        .type = ITEM_VULNERABILITY,
        .quantity = 1 + (size_t)rand() % 3
    });

    // Shuffle items.
    // https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
    for (size_t i = 0; i < inventory->count; ++i) {
        size_t j = i + (size_t)rand() % (inventory->count - i);
        Item item           = inventory->items[i];
        inventory->items[i] = inventory->items[j];
        inventory->items[j] = item;
    }
}

static void run_game(void) {
    Inventory required_items = {0};
    generate_required_items(&required_items);
    Map* map = generate_map(&required_items);

    long lose_time = get_time_s() + (long)required_items.count
        * SECONDS_PER_SYSTEM;

    Selector game_menu = {0};
    assert(0 < map->count);
    System* current_system = &map->systems[0];

    bool game_running = true;
    while (game_running) {
        long current_time = get_time_s();
        // TODO check time.

        // TODO handle ransomware.

        clear();
        // TODO: scan room.
        delayed_print(true, "%s", system_type_name(current_system->type));
        delayed_print(true, "Time left: %ld second(s)"
                          , lose_time - current_time);
        delayed_print_newline();

        selector_clear(&game_menu);

        // TODO add scan results.
        // Adds possible directions to move in.
        for (Direction direction = 0; direction < DIRECTION_COUNT; ++direction) {
            System* system = current_system->adjacent[direction];
            if (NULL == system) continue;

            switch (direction) {
            case DIRECTION_UP: {
                delayed_print(false, "[%s] is (U)P above", system_type_name(system->type));
                selector_add_option(&game_menu, 'u');
            } break;
            case DIRECTION_DOWN: {
                delayed_print(false, "[%s] is (D)OWN below", system_type_name(system->type));
                selector_add_option(&game_menu, 'd');
            } break;
            case DIRECTION_LEFT: {
                delayed_print(false, "[%s] is to the (L)EFT", system_type_name(system->type));
                selector_add_option(&game_menu, 'l');
            } break;
            case DIRECTION_RIGHT: {
                delayed_print(false, "[%s] is to the (R)IGHT", system_type_name(system->type));
                selector_add_option(&game_menu, 'r');
            } break;

            case DIRECTION_COUNT:
            default: assert(false && "unreachable");
            }
        }

        // Adds ability to take room's item.
        if (ITEM_NONE != current_system->item) {
            delayed_print(false, "There is a [%s]. (T)AKE it?"
                               , item_type_name(current_system->item));
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

        char choice = selector_get_selection(&game_menu);
        switch (choice) {
        case 'u': {
            current_system = current_system->adjacent[DIRECTION_UP];
        } break;
        case 'd': {
            current_system = current_system->adjacent[DIRECTION_DOWN];
        } break;
        case 'l': {
            current_system = current_system->adjacent[DIRECTION_LEFT];
        } break;
        case 'r': {
            current_system = current_system->adjacent[DIRECTION_RIGHT];
        } break;

        case 't': assert(false && "TODO");

        case 's': assert(false && "TODO");

        case 'i': assert(false && "TODO");

        case 'e': game_running = false; break;

        default: assert(false && "unreachable");
        }

        // TODO rest
    }

    free(map);
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

static void run_instructions_menu(void) {
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

static void run_about_menu(void) {
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

static void run_license_menu(void) {
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

static void run_exit_sequence(void) {
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

static void run_start_menu(void) {
    Selector start_menu = {0};
    selector_add_option(&start_menu, 'p'); // (P)LAY
    selector_add_option(&start_menu, 'i'); // (I)NSTRUCTIONS
    selector_add_option(&start_menu, 'a'); // (A)BOUT
    selector_add_option(&start_menu, 'l'); // (L)ICENSE
    selector_add_option(&start_menu, 'e'); // (E)XIT

    while (true) {
        clear();

        for (size_t line = 0; line < ARRAY_SIZE(logo); ++line) {
            delayed_print(true, "%s", logo[line]);
        }
        delayed_print_newline();
        delayed_print(true, "%s", version);
        delayed_print_newline();
        delayed_print(true, "Type and enter the character in paranthesis to "
                            "select an option.");
        delayed_print_newline();

        delayed_print(true, "(P)LAY");
        delayed_print(true, "(I)NSTRUCTIONS");
        delayed_print(true, "(A)BOUT");
        delayed_print(true, "(L)ICENSE");
        delayed_print(true, "(E)XIT");
        char choice = selector_get_selection(&start_menu);
        switch (choice) {
        case 'p': return;
        case 'i': run_instructions_menu(); break;
        case 'a': run_about_menu();        break;
        case 'l': run_license_menu();     break;
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
