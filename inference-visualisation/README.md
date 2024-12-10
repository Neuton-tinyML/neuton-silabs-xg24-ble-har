# Embedded model inference visualisation code

`src/main.py` handles visualisation of embedded Nuton model in silicon-labs-xg24-ble-har development board.

After connecting the dev-board to the computer, it is necessary to get the usb port id to which it is connected.

On a mac run `ls -l /dev/cu.usb*` command in terminal and all used ports will be displayed (e.g. /dev/cu.usbmodem0004402992621)

Replace your port id in the `PORT` variable inside `main.py` and run program with dev-board connected to computer via usb.

`python main.py`