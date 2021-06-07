#!/bin/bash
chmod 777 amebad_image_tool
echo
echo ==========
echo IMPORTANT
echo ==========
echo
echo Note: In some cases, you will be warned of executing thrid party executable file if you have never done so, dont worry, just change your OS setting to grant the permission
pause
echo -Firstly, set your ameba to UART Download mode by holding down UART Download button and press RESET button
echo
echo -Secondly, check your ameba serial port and type it down below, example: /dev/tty.usbserialxxxxxx
echo Serial Port:
read COM
./amebad_image_tool $COM
