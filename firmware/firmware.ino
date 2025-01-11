#include <PS2KeyAdvanced.h>

#include "keymap.h"

/**
 * adjust these to match your setup.
 */
const int datapin = 14;
const int irqpin = 12;
const int ledpin = 13;

/**
 * the default mode to start in. 0 means sending USB events, 1 means
 * serial monitoring. the modes are mutually exclusive because horrible
 * things happen if you have both on and focus the serial monitor
 * window.
 *
 * this could probably be detected but... nah. led operation is reversed
 * when in serial monitoring mode.
 */

#define MODE_SERIALMON 0
#define MODE_USB 1

int operating_mode = MODE_USB;

#define LED(x) ((operating_mode == MODE_SERIALMON) ? (x ? 0 : 1) : (x ? 1 : 0))

/**
 * the shortcut to toggle between the modes.
 */

const uint8_t SERIALMON_SHORTCUT_1 = 0x05; /* SysRq/attn */
const uint8_t SERIALMON_SHORTCUT_2 = 0x84; /* blank key at numpad top right */

const unsigned long reset_timeout = 1000;

/**
 * for now, the modifier keys are specified here.
 */
const uint8_t modifier_ctrl_l = 0x11;
const uint8_t modifier_ctrl_r = 0x58;
const uint8_t modifier_alt_l = 0x19;
const uint8_t modifier_alt_r = 0x39;
const uint8_t modifier_shift_l = 0x12;
const uint8_t modifier_shift_r = 0x59;
const uint8_t modifier_gui_l = 0x14;
const uint8_t modifier_gui_r = 0xef;


const uint8_t KEYB_DEFAULT_DISABLE = 0xf5;
const uint8_t KEYB_ECHO = 0xee;
const uint8_t KEYB_ENABLE = 0xf4;
const uint8_t KEYB_READ_ID = 0xf2;
const uint8_t KEYB_RESEND = 0xfe;
const uint8_t KEYB_RESET = 0xff;
const uint8_t KEYB_SET_ALL_ALL = 0xfa;
const uint8_t KEYB_SET_ALL_TYPEMATIC = 0xf7;
const uint8_t KEYB_SET_ALL_MAKE_BREAK = 0xf8;
const uint8_t KEYB_SET_ALL_MAKE = 0xf9;
const uint8_t KEYB_SET_DEFAULT = 0xf6;
const uint8_t KEYB_SET_TYPEMATIC_RATE_DELAY = 0xf3;

/** IBM PC AT Technical Reference, March 1986:
 *
 * Set Typematic Rate/Delay (Hex F3)
 *
 * The system issues the Set Typematic Rate/Delay command to change
 * the typematic rate and delay. The keyboard responds to the command
 * with ACK, stops scanning, and waits for the system to issue the
 * rate/ delay value byte. The keyboard responds to the rate/ delay
 * value byte with another ACK, sets the rate and delay to the values
 * indicated, and continues scanning (if it was previously
 * enabled). Bits 6 and 5 indicate the delay, and bits 4, 3, 2,1, and
 * 0 (the least-significant bit) the rate. Bit 7, the most-significant
 * bit, is always O. The delay is equal to 1 plus the binary value of
 * bits 6 and 5, multiplied by 250 milliseconds ± 20%.
 *
 * The period (interval from one typematic output to the next) is
 * determined by the following equation:
 *
 *   Period = (8 + A) X (2^B) X 0.00417 seconds
 *
 * where:
 *   A = binary value of bits 2, 1, and 0
 *   B = binary value of bits 4 and 3.
 *
 */

const uint8_t KEYB_REPLY_KEYERR_OVERRUN = 0;
const uint8_t KEYB_REPLY_BAT_COMPLETE = 0xaa;
const uint8_t KEYB_REPLY_BAT_FAILURE = 0xfc;
const uint8_t KEYB_REPLY_ECHO = 0xee;
const uint8_t KEYB_REPLY_ACK = 0xfa;
const uint8_t KEYB_REPLY_RESEND = 0xfe;

const uint8_t KEYB_CODE_BREAK = 0xf0;

const uint8_t HEXBUF_SIZE = 2;

const uint8_t KEYS = 240;

const uint8_t KEYSTATE_UP = 0;
const uint8_t KEYSTATE_DOWN = 1;
const uint8_t KEYSTATE_IGNORE = 2;

const uint8_t LED_ON_MS = 25;



PS2KeyAdvanced keyboard;

int error, hexin;

uint8_t held[6];
uint16_t modifiers;

uint8_t keystate[KEYS];

static char hexbuf[HEXBUF_SIZE];
static char *hexbufp;



uint16_t scancode_to_mod(uint8_t scancode) {
	switch (scancode) {
		case modifier_ctrl_l:
			return MODIFIERKEY_LEFT_CTRL;
		case modifier_ctrl_r:
			return MODIFIERKEY_RIGHT_CTRL;
		case modifier_alt_l:
			return MODIFIERKEY_LEFT_ALT;
		case modifier_alt_r:
			return MODIFIERKEY_RIGHT_ALT;
		case modifier_shift_l:
			return MODIFIERKEY_LEFT_SHIFT;
		case modifier_shift_r:
			return MODIFIERKEY_RIGHT_SHIFT;
		case modifier_gui_l:
			return MODIFIERKEY_LEFT_GUI;
		case modifier_gui_r:
			return MODIFIERKEY_RIGHT_GUI;
	default:
		return 0;
	}
}

void usb_release_all(void) {
	Keyboard.set_modifier(0);
	Keyboard.set_key1(0);
	Keyboard.set_key2(0);
	Keyboard.set_key3(0);
	Keyboard.set_key4(0);
	Keyboard.set_key5(0);
	Keyboard.set_key6(0);
	Keyboard.send_now();
}

void add_held(uint8_t scancode) {
	int p = -1;
	for (int i = 0; i < 6; i++) {
		if (!held[i])
			p = i;
		if (held[i] == scancode)
			return;
	}
	held[p] = scancode;
}

void del_held(uint8_t scancode) {
	for (int i = 0; i < 6; i++) {
		if (held[i] == scancode) {
			held[i] = 0;
			return;
		}
	}
}

void init_held(void) {
	memset(&held, 0, 6);
}

void sendbyte(uint8_t send) {
	if (operating_mode == MODE_SERIALMON)
		Serial.printf("\r     sent >>\t0x%02x\n", send);
	keyboard.sendbyte(send);
}

void print_code(int scancode) {
	if (operating_mode != MODE_SERIALMON)
		return;
	
	if (keymap[scancode].scancode != 0) {
		if (keystate[scancode]) {
			Serial.printf("  pressed");
		} else {
			Serial.printf(" released");
		}
		if (keymap[scancode].usage_id)
			Serial.printf(" <<\t0x%02x => 0x%02x", scancode, keymap[scancode].usage_id);
		else
			Serial.printf(" <<\t0x%02x => (none)", scancode);
			
		if (keymap[scancode].name)
			Serial.printf(" = %s", keymap[scancode].name);
	} else if (!error) {
		if (scancode != KEYB_CODE_BREAK)
			Serial.printf("    reply <<\t0x%02x", scancode);
			
		switch (scancode) {
		case KEYB_CODE_BREAK:
			goto out;
		case KEYB_REPLY_ACK:
			Serial.printf("\tACK");
			break;
		case KEYB_REPLY_ECHO:
			Serial.printf("\tECHO");
			break;
		case KEYB_REPLY_BAT_COMPLETE:
			Serial.printf("\tBAT OK");
			break;
		case KEYB_REPLY_BAT_FAILURE:
			Serial.printf("\tBAT FAIL");
			break;
		case KEYB_REPLY_KEYERR_OVERRUN:
			Serial.printf("\tKEY DETECTION ERROR/OVERRUN");
			error = 1;
			break;
		default:
			;
		}
	}
	Serial.printf("\n");
 out:
	;
}

int process_code(void) {
	static int breaking;
	int scancode = 0;
	
	if (keyboard.available()) {
		scancode = keyboard.read();

		if (keystate[scancode] == KEYSTATE_IGNORE)
			return scancode;

		if (scancode < 0xa0 && keymap[scancode].scancode == 0) {
			if (operating_mode == MODE_SERIALMON)
				print_code(scancode);
			return scancode;
		}
		
		switch (scancode) {
		case KEYB_CODE_BREAK:
			breaking = 1;
			break;
		case modifier_ctrl_l:
		case modifier_ctrl_r:
		case modifier_alt_l:
		case modifier_alt_r:
		case modifier_shift_l:
		case modifier_shift_r:
		case modifier_gui_l:
		case modifier_gui_r:
			if (breaking) {
				keystate[scancode] = KEYSTATE_UP;
				modifiers &= ~scancode_to_mod(scancode);
				breaking = 0;
			} else {
				keystate[scancode] = KEYSTATE_DOWN;
				modifiers |= scancode_to_mod(scancode);
			}
			break;
		default:
			if (breaking) {
				keystate[scancode] = KEYSTATE_UP;
				breaking = 0;
				del_held(scancode);
			} else {
				keystate[scancode] = KEYSTATE_DOWN;
				add_held(scancode);
			}
		}

		if (operating_mode == MODE_SERIALMON) {
			print_code(scancode);
		} else {
			Keyboard.set_modifier(modifiers);
			Keyboard.set_key1(held[0] ? keymap[held[0]].usage_id : 0);
			Keyboard.set_key2(held[1] ? keymap[held[1]].usage_id : 0);
			Keyboard.set_key3(held[2] ? keymap[held[2]].usage_id : 0);
			Keyboard.set_key4(held[3] ? keymap[held[3]].usage_id : 0);
			Keyboard.set_key5(held[4] ? keymap[held[4]].usage_id : 0);
			Keyboard.set_key6(held[5] ? keymap[held[5]].usage_id : 0);

			Keyboard.send_now();
		}
	}

	return scancode;
}

int wait_for_ack(void) {
	unsigned long then;
	then = millis();

	while(process_code() != KEYB_REPLY_ACK) {
		digitalWriteFast(ledpin, LED(millis() / 1000 % 2));
		if (millis() - then > reset_timeout) {
			if (operating_mode == MODE_SERIALMON)
				Serial.printf("timed out...\n");
			return 0;
		}
		delay(1);
	};

	return 1;
}

void reset_kb(void) {
	digitalWriteFast(ledpin, LED(1));

 again:
	sendbyte(KEYB_RESET);
	if (!wait_for_ack())
		goto again;

	sendbyte(KEYB_SET_ALL_MAKE_BREAK);
	if (!wait_for_ack())
		goto again;
	
	sendbyte(KEYB_ENABLE);
	if (!wait_for_ack())
		goto again;

	digitalWriteFast(ledpin, LED(0));
}

void process_serial() {
	int n;
	unsigned char send;

	send = 0;
	n = 0;
	
	if (Serial.available()) {
		char c = Serial.read();
		if (!hexin) {
			switch (c) {
			case 'r':
			case 'R':
				init_held();
				modifiers = 0;
				reset_kb();
				break;
			case 'i':
			case 'I':
				sendbyte(KEYB_READ_ID);
				break;
			case 's':
			case 'S':
				Serial.printf("hex to send? ");
				hexin = 1;
				break;
			case 'x':
			case 'X':
				for (int i = 0; i < KEYS; i++) {
					if (keystate[i]) {
						n++;
						Serial.printf(" now being held: 0x%02x", i);
						if (keymap[i].name) {
							Serial.printf(" = %s", keymap[i].name);								
						}
						Serial.printf("\n");
					}
				}
				if (!n)
					Serial.printf(" nothing being held\n");
				break;
			default:
				;
			}
		} else {
			if (c == '\033' || c == ' ') {
				Serial.printf("\rcancel!              \r");
				hexin = 0;
				hexbufp = hexbuf;
				memset(&hexbuf, 0, HEXBUF_SIZE);
			} else if (c == '\r' || hexbufp - hexbuf > 0) {
				*hexbufp = c;
				if (!sscanf(hexbuf, "%02hhx", &send)
						&& !sscanf(hexbuf, "%02hhX", &send)) {
					Serial.printf("\rinvalid input hex value!\n");
				} else {
					Serial.printf("\r                            \r");
					sendbyte(send);
				}
				hexin = 0;
				hexbufp = hexbuf;
				memset(&hexbuf, 0, HEXBUF_SIZE);
			} else {
				*hexbufp = c;
				hexbufp++;
				Serial.printf("%c", c);
			}
		}
	}
}

void setup() {
	static int waiting;
	unsigned long led_on;

	led_on = 0;
	error = hexin = 0;
	init_held();
	modifiers = 0;
	memset(&hexbuf, 0, HEXBUF_SIZE);
	memset(&keystate, 0, sizeof keystate);
	hexbufp = hexbuf;

	/**
	 * if you had a stuck key with scancode 0x04, this is how you'd tell the
	 * firmware to ignore the key.
	 */
	/* keystate[0x04] = KEYSTATE_IGNORE; */

	Serial.begin(115200);
	
	pinMode(ledpin, OUTPUT);
	keyboard.begin(datapin, irqpin);

	if (operating_mode == MODE_SERIALMON)
		Serial.printf("\nready, resetting kb\n\n");
	
	reset_kb();
	
	while (1) {
		if (process_code()) {
			digitalWriteFast(ledpin, LED(1));
			led_on = millis();
		} else if (led_on && millis() - led_on > LED_ON_MS) {
			digitalWriteFast(ledpin, LED(0));
			led_on = 0;
		}

		if (operating_mode == MODE_SERIALMON)
			process_serial();

		if (keystate[SERIALMON_SHORTCUT_1] == KEYSTATE_DOWN
				&& keystate[SERIALMON_SHORTCUT_2] == KEYSTATE_DOWN) {

			usb_release_all();
				
			if (waiting)
				continue;
			
			waiting = 1;
			
			if (operating_mode == MODE_SERIALMON) {
				Serial.printf("\n\nresume serial monitor by pressing %s + %s\n\n\t\t...bye!\n", keymap[0x01].name, keymap[0x4d].name);
				operating_mode = MODE_USB;
			} else {
				Serial.printf("\n\ngreetings, stranger.\n\n");
				operating_mode = MODE_SERIALMON;
			}

		} else if (waiting) {
			waiting = 0;
		}
	}
}

void loop() {
}
