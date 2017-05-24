// vim: ts=4 sw=4 sts=4
/* ----------------------------------------------------------------------------
 * main()
 * ----------------------------------------------------------------------------
 * Copyright (c) 2012 Ben Blazak <benblazak.dev@gmail.com>
 * Released under The MIT License (MIT) (see "license.md")
 * Project located at <https://github.com/benblazak/ergodox-firmware>
 * ------------------------------------------------------------------------- */


#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>
#include "usb_keyboard_rawhid.h"
//#include "./lib-other/pjrc/usb_keyboard/usb_keyboard.h"
#include "./lib/key-functions/public.h"
#include "./keyboard/controller.h"
#include "./keyboard/layout.h"
#include "./keyboard/matrix.h"
#include "./main.h"

#include <avr/pgmspace.h>

#undef KB_LAYERS
#define KB_LAYERS   5
#define KB_ROWS     6
#define KB_COLUMNS 14

// http://www.mindrunway.ru/IgorPlHex/USBKeyScan.pdf
// https://www.arduino.cc/en/Reference/KeyboardModifiers
#define MLCT 0xE0
#define MLSH 0xE1
#define MLAL 0xE2
#define MLGU 0xE3
#define MRCT 0xE4
#define MRSH 0xE5
#define MRAL 0xE6
#define MRGU 0xE7

// Previous ones after MLGU: 0x35, 0x64, 0x5C, 0x5E
//     5  6  7 8
//        4  9
//  3, 2, 1  C B A    (1=C, 2=S, 4=A, 8=G)
const uint16_t PROGMEM custom_layout[KB_LAYERS][KB_ROWS][KB_COLUMNS] = {
    {{0x0029, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x002E,      0x0000, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x002D },
     {0x002B, 0x0014, 0x001A, 0x0008, 0x0015, 0x0017, 0x0225,      0x002F, 0x001C, 0x0018, 0x000C, 0x0012, 0x0013, 0x0030 },
     {0x0031, 0x0004, 0x0016, 0x0007, 0x0009, 0x000A,      0,           0, 0x000B, 0x000D, 0x000E, 0x000F, 0x0033, 0x0034 },
     {  MLSH, 0x001D, 0x001B, 0x0006, 0x0019, 0x0005, 0x0000,      0x0000, 0x0011, 0x0010, 0x0036, 0x0037, 0x0038,   MRSH },
     {  MLGU, 0x0065, 0x002E, 0x0035, 0x0064,      0,      0,           0,      0, 0x0050, 0x0051, 0x0052, 0x004F,   MRGU },
     {     0, 0x004D, 0x004C, 0x002A, 0x004A,   MLCT,   MLAL,        MRAL,   MRCT, 0x004B, 0x002C, 0x0028, 0x004E,      0 }},

    {{0x0029, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x002E,      0x0000, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x002D },
     {0x002B, 0x201E, 0x201F, 0x2020, 0x2021, 0x2022, 0x0044,      0x0045, 0x401E, 0x401F, 0x4020, 0x4021, 0x4022, 0x0030 },
     {0x0031, 0x2023, 0x2024, 0x2025, 0x2026, 0x2027,      0,           0, 0x4023, 0x4024, 0x4025, 0x4026, 0x4027, 0x0034 },
     {  MLSH, 0x001D, 0x001B, 0x0006, 0x0019, 0x0005, 0x0000,      0x0000, 0x0011, 0x0010, 0x0036, 0x0037, 0x0038,   MRSH },
     {0x043B, 0x0065, 0x002E, 0x0035, 0x0064,      0,      0,           0,      0, 0x0550, 0x042B, 0x062B, 0x054F, 0x043D },
     {     0, 0x004D, 0x004C, 0x002A, 0x004A,   MLCT,   MLAL,        MRAL,   MRCT, 0x004B, 0x002C, 0x0028, 0x004E,      0 }},

    {{0x0029, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x002E,      0x0000, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x002D },
     {0x002B, 0x0014, 0x001A, 0x0008, 0x0015, 0x0017, 0x0225,      0x002F, 0x001C, 0x0018, 0x000C, 0x0012, 0x0013, 0x0030 },
     {0x0031, 0x0004, 0x0016, 0x0007, 0x0009, 0x000A,      0,           0, 0x000B, 0x000D, 0x000E, 0x000F, 0x0033, 0x0034 },
     {  MLSH, 0x001D, 0x001B, 0x0006, 0x0019, 0x0005, 0x0000,      0x0000, 0x0011, 0x0010, 0x0036, 0x0037, 0x0038,   MRSH },
     {  MLGU, 0x0065, 0x002E, 0x0035, 0x0064,      0,      0,           0,      0, 0x0050, 0x0051, 0x0052, 0x004F,   MRGU },
     {     0, 0x004D, 0x004C, 0x002A, 0x004A,   MLCT,   MLAL,        MRAL,   MRCT, 0x004B, 0x002C, 0x0028, 0x004E,      0 }},

    {{0x0029, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x002E,      0x0000, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x002D },
     {0x002B, 0x201E, 0x201F, 0x2020, 0x2021, 0x2022, 0x0044,      0x0045, 0x401E, 0x401F, 0x4020, 0x4021, 0x4022, 0x0030 },
     {0x0031, 0x2023, 0x2024, 0x2025, 0x2026, 0x2027,      0,           0, 0x4023, 0x4024, 0x4025, 0x4026, 0x4027, 0x0034 },
     {  MLSH, 0x001D, 0x001B, 0x0006, 0x0019, 0x0005, 0x0000,      0x0000, 0x0011, 0x0010, 0x0036, 0x0037, 0x0038,   MRSH },
     {0x043B, 0x0065, 0x002E, 0x0035, 0x0064,      0,      0,           0,      0, 0x0550, 0x042B, 0x062B, 0x054F, 0x043D },
     {     0, 0x004D, 0x004C, 0x002A, 0x004A,   MLCT,   MLAL,        MRAL,   MRCT, 0x004B, 0x002C, 0x0028, 0x004E,      0 }},

    {{0x0029, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x002E,      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x002D },
     {0x002B, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x4025,      0x002F, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x0030 },
     {0x0031, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043,      0,           0, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0034 },
     {  MLSH, 0x0044, 0x0045, 0x0006, 0x0019, 0x0005, 0x0000,      0x0000, 0x0011, 0x0010, 0x0036, 0x0037, 0x0038,   MRSH },
     {  MLGU, 0x0065, 0x002E, 0x0035, 0x0064,      0,      0,           0,      0, 0x0050, 0x0051, 0x0052, 0x004F,   MRGU },
     {     0, 0x004D, 0x004C, 0x002A, 0x004A,   MLCT,   MLAL,        MRAL,   MRCT, 0x004B, 0x002C, 0x0028, 0x004E,      0 }}
};


// ----------------------------------------------------------------------------

#define  MAX_ACTIVE_LAYERS  20

// ----------------------------------------------------------------------------

static bool _main_kb_is_pressed[KB_ROWS][KB_COLUMNS];
bool (*main_kb_is_pressed)[KB_ROWS][KB_COLUMNS] = &_main_kb_is_pressed;

static bool _main_kb_was_pressed[KB_ROWS][KB_COLUMNS];
bool (*main_kb_was_pressed)[KB_ROWS][KB_COLUMNS] = &_main_kb_was_pressed;

// static bool main_kb_was_transparent[KB_ROWS][KB_COLUMNS];

uint8_t main_layers_pressed[KB_ROWS][KB_COLUMNS];

uint8_t main_loop_row;
uint8_t main_loop_col;

uint8_t main_arg_layer;
uint8_t main_arg_layer_offset;
uint8_t main_arg_row;
uint8_t main_arg_col;
bool    main_arg_is_pressed;
bool    main_arg_was_pressed;
bool    main_arg_any_non_trans_key_pressed;
bool    main_arg_trans_key_pressed;

// 
uint8_t main_l_mode;
uint8_t main_r_mode;
uint8_t main_key_modifiers;
uint8_t main_direct_modifiers;

// ----------------------------------------------------------------------------
// uint8_t usb_rawhid_fill = 0;
// uint8_t usb_rawhid_buffer[64];

/*
 * main()
 */
int main(void) {
	kb_init();  // does controller initialization too
    //teensy_init(); // return 1
    //mcp23018_init(); // return 2

	kb_led_state_power_on();

	usb_init();
	while (!usb_configured());
	kb_led_delay_usb_init();  // give the OS time to load drivers, etc.

	kb_led_state_ready();

    main_l_mode = 0;
    main_r_mode = 0;
    main_key_modifiers = 0;
    main_direct_modifiers = 0;

	for (;;) {
		// swap `main_kb_is_pressed` and `main_kb_was_pressed`, then update
		bool (*temp)[KB_ROWS][KB_COLUMNS] = main_kb_was_pressed;
		main_kb_was_pressed = main_kb_is_pressed;
		main_kb_is_pressed = temp;

		kb_update_matrix(*main_kb_is_pressed);

		// this loop is responsible to
		// - "execute" keys when they change state
		// - keep track of which layers the keys were on when they were pressed
		//   (so they can be released using the function from that layer)
		//
		// note
		// - everything else is the key function's responsibility
		//   - see the keyboard layout file ("keyboard/ergodox/layout/*.c") for
		//     which key is assigned which function (per layer)
		//   - see "lib/key-functions/public/*.c" for the function definitions
		#define row          main_loop_row
		#define col          main_loop_col
		#define layer        main_arg_layer
		#define is_pressed   main_arg_is_pressed
		#define was_pressed  main_arg_was_pressed
        //if ((*main_kb_is_pressed)[5][0] > (*main_kb_was_pressed)[5][0]) {
        //    main_l_mode = 0;
        //    main_r_mode = 0;
        //}
        //else {
		//	for (col=0; col<5; col++) {
        //        if ((*main_kb_is_pressed)[5][1+col] > (*main_kb_was_pressed)[5][1+col]) {
        //            main_l_mode = col;
        //        }
        //        if ((*main_kb_is_pressed)[5][8+col] > (*main_kb_was_pressed)[5][8+col]) {
        //            main_r_mode = col;
        //        }
        //    }
        //}
        uint8_t mode = main_l_mode;

        #define GOING_DOWN(row, col) ((*main_kb_is_pressed)[row][col] > (*main_kb_was_pressed)[row][col])
        #define PRESSED(row, col) (*main_kb_is_pressed)[row][col]
        if (GOING_DOWN(5, 7)) main_l_mode ^= 2;
        if (PRESSED(2, 6)) mode = 1;
        if (PRESSED(2, 7)) mode = 1;

		for (row=0; row<KB_ROWS; row++) {
			for (col=0; col<KB_COLUMNS; col++) {
				is_pressed = (*main_kb_is_pressed)[row][col];
				was_pressed = (*main_kb_was_pressed)[row][col];

                //uint8_t mode = (col < KB_COLUMNS/2)?main_l_mode:main_r_mode;

				if (is_pressed != was_pressed) {
                    uint16_t kc = pgm_read_word(&custom_layout[mode][KB_ROWS - 1 - row][col]);
                    uint8_t h = (kc >> 8);
                    uint8_t c = (kc & 255);

					if (is_pressed) {
                        if ((c & 0xE8) == 0xE0) {
                            main_direct_modifiers |= (1 << (c & 7));
                        } else {
                            main_key_modifiers = h;
                            // If they key was already pressed, "repeat" the keypress.
                            for (uint8_t i = 0; i < 6; i++) {
                                if (keyboard_keys[i] == c) {
                                    keyboard_keys[i] = 0;
                                    usb_keyboard_send();
                                    break;
                                }
                            }
                            for (uint8_t i = 0; i < 6; i++) {
                                if (keyboard_keys[i] == 0) {
                                    keyboard_keys[i] = c;
                                    break;
                                }
                            // Otherwise here, the buffer send would have to maintain the modified semantics.
                            // TODO: Consider how to handle the simultaneous modifier presses.
                            }
                        }
                    } else {
                        if ((c & 0xE8) == 0xE0) {
                            main_direct_modifiers &= ~(1 << (c & 7));
                        } else {
                            uint8_t clean = 1;
                            // The last pressed modifier is preserved
                            // If all keys are released, the modifier is released.
                            for (uint8_t i = 0; i < 6; i++) {
                                if (keyboard_keys[i] == c) {
                                    keyboard_keys[i] = 0;
                                }
                                clean &= (keyboard_keys[i] > 0); 
                            }
                            if (clean) {
                                main_key_modifiers = 0;
                            }
                        }
                    }
                    // Modifier key press causes modifier inversion.
                    keyboard_modifier_keys = main_direct_modifiers ^ main_key_modifiers;

//                  if (is_pressed) {
//						layer = main_layers_peek(0);
//						main_layers_pressed[row][col] = layer;
//						main_arg_trans_key_pressed = false;
//					} else {
//						layer = main_layers_pressed[row][col];
//						main_arg_trans_key_pressed = main_kb_was_transparent[row][col];
//					}

					// set remaining vars, and "execute" key
					// main_arg_row          = row;
					// main_arg_col          = col;
					// main_arg_layer_offset = 0;
					// main_exec_key();
					// main_kb_was_transparent[row][col] = main_arg_trans_key_pressed;

                    // usb_rawhid_buffer[usb_rawhid_fill++] = is_pressed ? 1 : 2;
                    // usb_rawhid_buffer[usb_rawhid_fill++] = col*KB_COLUMNS + row;
                    // if (usb_rawhid_fill >= 64) {
                    //     usb_rawhid_send(usb_rawhid_buffer, 0);
                    //     usb_rawhid_fill = 0;
                    // }
				}
			}
		}
		#undef row
		#undef col
		#undef layer
		#undef is_pressed
		#undef was_pressed

		// send the USB report (even if nothing's changed)
		usb_keyboard_send();
		usb_extra_consumer_send();
       
        // This addition sends rawhid packet if it is filled up.
        // if (usb_rawhid_fill > 0) {
        //     while (usb_rawhid_fill < 64) usb_rawhid_buffer[usb_rawhid_fill++] = 0;
        //     usb_rawhid_send(usb_rawhid_buffer, 0);
        //     usb_rawhid_fill = 0;
        // }

		_delay_ms(MAKEFILE_DEBOUNCE_TIME);

		// update LEDs
		if (keyboard_leds & (1<<0)) { kb_led_num_on(); }
		else { kb_led_num_off(); }
		if (keyboard_leds & (1<<1)) { kb_led_caps_on(); }
		else { kb_led_caps_off(); }
		if (keyboard_leds & (1<<2)) { kb_led_scroll_on(); }
		else { kb_led_scroll_off(); }
		if (keyboard_leds & (1<<3)) { kb_led_compose_on(); }
		else { kb_led_compose_off(); }
		if (keyboard_leds & (1<<4)) { kb_led_kana_on(); }
		else { kb_led_kana_off(); }
	}

	return 0;
}

// ----------------------------------------------------------------------------

// convenience macros (for the helper functions below)
// #define  layer        main_arg_layer
// #define  row          main_arg_row
// #define  col          main_arg_col
// #define  is_pressed   main_arg_is_pressed
// #define  was_pressed  main_arg_was_pressed

// ----------------------------------------------------------------------------

/* ----------------------------------------------------------------------------
 * Layer Functions
 * ----------------------------------------------------------------------------
 * We keep track of which layer is foremost by placing it on a stack.  Layers
 * may appear in the stack more than once.  The base layer will always be
 * layer-0.  
 *
 * Implemented as a fixed size stack.
 * ------------------------------------------------------------------------- */

// ----------------------------------------------------------------------------

// struct layers {
// 	uint8_t layer;
// 	uint8_t id;
// 	uint8_t sticky;
// };
// 
// // ----------------------------------------------------------------------------
// 
// struct layers layers[MAX_ACTIVE_LAYERS];
// uint8_t       layers_head = 0;
// uint8_t       layers_ids_in_use[MAX_ACTIVE_LAYERS] = {true};
// 
// /*
//  * Exec key
//  * - Execute the keypress or keyrelease function (if it exists) of the key at
//  *   the current possition.
//  */
// void main_exec_key(void) {
// 	void (*key_function)(void) =
// 		( (is_pressed)
// 		  ? kb_layout_press_get(layer, row, col)
// 		  : kb_layout_release_get(layer, row, col) );
// 
// 	if (key_function)
// 		(*key_function)();
// 
// 	// If the current layer is in the sticky once up state and a key defined
// 	//  for this layer (a non-transparent key) was pressed, pop the layer
// 	if (layers[layers_head].sticky == eStickyOnceUp && main_arg_any_non_trans_key_pressed)
// 		main_layers_pop_id(layers_head);
// }
// 
// /*
//  * peek()
//  *
//  * Arguments
//  * - 'offset': the offset (down the stack) from the head element
//  *
//  * Returns
//  * - success: the layer-number of the requested element (which may be 0)
//  * - failure: 0 (default) (out of bounds)
//  */
// uint8_t main_layers_peek(uint8_t offset) {
// 	if (offset <= layers_head)
// 		return layers[layers_head - offset].layer;
// 
// 	return 0;  // default, or error
// }
// 
// uint8_t main_layers_peek_sticky(uint8_t offset) {
// 	if (offset <= layers_head)
// 		return layers[layers_head - offset].sticky;
// 
// 	return 0;  // default, or error
// }
// 
// /*
//  * push()
//  *
//  * Arguments
//  * - 'layer': the layer-number to push to the top of the stack
//  *
//  * Returns
//  * - success: the id assigned to the newly added element
//  * - failure: 0 (the stack was already full)
//  */
// uint8_t main_layers_push(uint8_t layer, uint8_t sticky) {
// 	// look for an available id
// 	for (uint8_t id=1; id<MAX_ACTIVE_LAYERS; id++) {
// 		// if one is found
// 		if (layers_ids_in_use[id] == false) {
// 			layers_ids_in_use[id] = true;
// 			layers_head++;
// 			layers[layers_head].layer = layer;
// 			layers[layers_head].id = id;
// 			layers[layers_head].sticky = sticky;
// 			return id;
// 		}
// 	}
// 
// 	return 0;  // default, or error
// }
// 
// /*
//  * pop_id()
//  *
//  * Arguments
//  * - 'id': the id of the element to pop from the stack
//  */
// void main_layers_pop_id(uint8_t id) {
// 	// look for the element with the id we want to pop
// 	for (uint8_t element=1; element<=layers_head; element++) {
// 		// if we find it
// 		if (layers[element].id == id) {
// 			for(; element<layers_head; ++element) {
// 				layers[element].layer = layers[element+1].layer;
// 				layers[element].id = layers[element+1].id;
// 			}
// 			// reinitialize the topmost (now unused) slot
// 			layers[layers_head].layer = 0;
// 			layers[layers_head].id = 0;
// 			// record keeping
// 			layers_ids_in_use[id] = false;
// 			layers_head--;
// 			return;
// 		}
// 	}
// }
// 
// /* ----------------------------------------------------------------------------
//  * ------------------------------------------------------------------------- */
// 
