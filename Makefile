OBJS		=	readmap.o 
CFLAGS		=	-g
ARDUINO_CLI	=	arduino-cli
ARDUINO_FQBN	=	teensy:avr:teensy31
ARDUINO_PORT	=	"1:7"
KEYMAP_FILE	=	data/1387901-fi-sw.txt

mkheader:	${OBJS} mkheader.o
all:		fiswmap.h firmware upload
keymap.h:	mkheader
		./mkheader keymap < $(KEYMAP_FILE) > keymap.h
firmware:	keymap.h
		cp -f keymap.h m122usbcon/
		cp -f mapping.h m122usbcon/
		cd m122usbcon/; \
		$(ARDUINO_CLI) compile --fqbn $(ARDUINO_FQBN) \
		--library ./PS2KeyAdvanced

upload:		firmware
		cd m122usbcon/; \
		$(ARDUINO_CLI) upload --fqbn $(ARDUINO_FQBN) \
		--port $(ARDUINO_PORT)

clean:		rm *.o mkheader
