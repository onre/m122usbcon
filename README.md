# m122usbcon - IBM M122 USB adapter firmware

## What is it?

It's a minimal firmware for connecting an IBM 122-key terminal keyboard to a USB host using a Teensy microcontroller as the intermediator. It includes a version of [PS2KeyAdvanced](https://github.com/techpaul/PS2KeyAdvanced.git) library modified for scancode set 3 operation. It probably works with any scancode set 3 keyboard.

I wrote it because I did not want to modify the keyboard in any way, and did not have the required hardware for the other similar firmwares.

## How to use it?

The external dependencies are `make`, a C compiler for the host and `arduino-cli` with Teensy support libraries installed. If it is not in `$PATH`, edit the Makefile accordingly.

Create a keymap description in the `data/` directory. See the provided example file for syntax. It describes the IBM part number 1387901 with Finnish/Swedish layout, which is what I have. The scancode map on [this page](https://www.seasip.info/VintagePC/ibm_1390876.html) is correct for this unit. For USB HID usage IDs you can refer to [the official specification](https://usb.org/sites/default/files/hut1_5.pdf) from page 89 onwards.

When you're done, edit the Makefile and point the `KEYMAP_FILE` variable to the keymap you just created. Now, execute `make` in the top-level directory. This should create a `keymap.h` header file from your keymap, build a firmware image using the newly-created header and upload the result to the microcontroller.

## The serial monitor

If you need to debug your keyboard, this feature can be useful. To use it, build and install the firmware and then tell your communications software to connect to the microcontroller. If you use [C-Kermit](https://www.kermitproject.org/), you can use the included `serialmon.krm` script. You may need to adjust the port names to match your system.

Once the connection is established, press the shortcut defined in `firmware/firmware.ino` to activate the monitor. The default shortcut is the extreme left and right keys of the number row.

The monitor accepts a few commands. Press `r` to reset the keyboard, `i` to ask for ID, `x` to show which keys are currently being held and `s` to send arbitrary byte of data to it, input in hex. You can exit the hex input by pressing `Esc` or `Spacebar`.

## Getting all keys to produce distinct keycodes under Linux

If you're getting keycode F0 (decimal 240) from multiple keys, you will probably need to add some scancode-to-keycode mappings manually.

First, edit the included  `90-m122.hwdb` to your liking. You can use `evtest` to see the scancodes, and this one-liner will give you a sorted list of key names:

```
$ grep 'define KEY_' /usr/include/linux/input-event-codes.h|cut -d_ -f2|awk '{print $1;}'|sort|uniq|tr [A-Z] [a-z]
```

Once you're done, install the rules, rebuild the hardware database and tell udevd to reload it.

As root:
```
# cp 90-m122.hwdb /etc/udev/hwdb.d
# systemd-hwdb update
# udevadm trigger
```

## TODO

- general code cleanup
- allow simultaneous USB output and serial monitoring with loopback detection
- get rid of Arduino dependency
- as a part of that, get rid of the C++ PS/2 library too

## License

Contents of the `m122usbcon/PS2KeyAdvanced` directory are under LGPL and all the rest is MIT. See the LICENSE files in `m122usbcon/PS2KeyAdvanced` and this directory for more information.

