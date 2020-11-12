@echo ===========================
@echo !                         !
@echo !        IMPORTANT        !
@echo !                         !
@echo ===========================
@echo First check your ameba's serial port and edit this file accordingly before setting your ameba to UART Download mode and upload the image
@echo If your ameba's COM port is not "COM8", right click on this file and select "Edit", update the correct COM port to the third last line then save and exit before re-run this program 
pause
amebad_image_tool.exe COM20
:: Change COM8 to your ameba's COM port
pause