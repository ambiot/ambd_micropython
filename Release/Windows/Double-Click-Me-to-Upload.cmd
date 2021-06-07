@echo ===========================
@echo !                         !
@echo !        IMPORTANT        !
@echo !                         !
@echo ===========================
@echo First check your ameba's serial port and edit this file accordingly before setting your ameba to UART Download mode and upload the image
@echo Right click on this file and select "Edit", update the correct COM port to the third last line then save and exit before re-run this program 
pause
amebad_image_tool.exe COM8
:: Change COMXX above to your ameba's actual COM port number
pause
