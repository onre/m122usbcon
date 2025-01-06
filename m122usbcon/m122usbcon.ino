#include <PS2KeyAdvanced.h>

#include "keymap.h"

#define DEBUG

/**
 * technical quotes are from IBM PC AT Technical Reference, March 1986
 */

const int datapin = 1;
const int irqpin = 0;
const int ledpin = 13;

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

/** Set Typematic Rate/Delay (Hex F3)
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
 */

const uint8_t KEYB_REPLY_KEYERR_OVERRUN = 0;
const uint8_t KEYB_REPLY_BAT_COMPLETE = 0xaa;
const uint8_t KEYB_REPLY_BAT_FAILURE = 0xfc;
const uint8_t KEYB_REPLY_ECHO = 0xee;
const uint8_t KEYB_REPLY_ACK = 0xfa;
const uint8_t KEYB_REPLY_RESEND = 0xfe;

const uint8_t KEYB_CODE_BREAK = 0xf0;

PS2KeyAdvanced keyboard;

#define HEXBUF_SIZE 2

int error, hexin, breaking;

#define KEYS 240

int keystate[KEYS];

void sendbyte(uint8_t send) {
	Serial.printf("\r    send >>\t0x%02x\n", send);
	keyboard.sendbyte(send);
}

int print_code(void) {
	int scancode = 0;
	
	if (keyboard.available()) {
		scancode = keyboard.read();
		if (keymap[scancode].scancode != 0) {
			if (breaking) {
				Serial.printf("released");
				keystate[scancode] = 0;
				breaking = 0;
			} else {
				Serial.printf("pressed ");
				keystate[scancode] = 1;
			}
			Serial.printf(" <<\t0x%02x", scancode);
			
			if (keymap[scancode].name)
				Serial.printf(" = %s", keymap[scancode].name);
		} else if (!error) {
			if (scancode != KEYB_CODE_BREAK)
				Serial.printf("    data <<\t0x%02x", scancode);
			
				switch (scancode) {
				case KEYB_CODE_BREAK:
					breaking = 1;
					break;
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
		if (scancode != KEYB_CODE_BREAK)
			Serial.printf("\n");
	}

	return scancode;
}

void reset_kb(void) {
	sendbyte(KEYB_RESET);
	while(print_code() != KEYB_REPLY_ACK);

	sendbyte(KEYB_SET_ALL_MAKE_BREAK);
	while(print_code() != KEYB_REPLY_ACK);

	sendbyte(KEYB_ENABLE);
	while(print_code() != KEYB_REPLY_ACK);
}

void setup() {
	int n;
	unsigned char send;
	char hexbuf[HEXBUF_SIZE];
	char *hexbufp;

	error = hexin = send = breaking = 0;
	memset(&hexbuf, 0, HEXBUF_SIZE);
	memset(&keystate, 0, sizeof keystate);
	hexbufp = hexbuf;

	Serial.begin(115200);
	
	pinMode(ledpin, OUTPUT);
	keyboard.begin(datapin, irqpin);

	reset_kb();
	
	while (1) {
		n = 0;
		print_code();

		if (Serial.available()) {
			char c = Serial.read();
			if (!hexin) {
				switch (c) {
				case 'r':
				case 'R':
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
}

void loop() {
}
