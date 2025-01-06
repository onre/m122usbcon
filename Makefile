
ARDUINO_FQBN		=	teensy:avr:teensy31
ARDUINO_PORT		=	"1"
ARDUINO_BOARDOPTS	=	speed=48,usb=serialhid

KEYMAP_FILE		=	data/1387901-fi-sw.txt


OBJS			=	readmap.o mkheader.o
CFLAGS			=	-g
ARDUINO_CLI		=	arduino-cli
ARDUINO_BUILDPATH	=	build/


all:		keymap.h firmware upload

mkheader:	${OBJS}

keymap.h:	mkheader
		./mkheader keymap < $(KEYMAP_FILE) > $@

firmware:	keymap.h
		cp -f keymap.h firmware/
		cp -f mapping.h firmware/
		cd firmware/; \
		$(ARDUINO_CLI) compile --fqbn $(ARDUINO_FQBN) \
		--board-options $(ARDUINO_BOARDOPTS) \
		--library ./PS2KeyAdvanced

upload:		firmware
		cd firmware/; \
		$(ARDUINO_CLI) upload --fqbn $(ARDUINO_FQBN) \
		--port $(ARDUINO_PORT)

clean:
		rm -f *.o mkheader keymap.h firmware/keymap.h firmware/mapping.h
