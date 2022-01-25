@echo  ===========================
@echo  !                         !
@echo  !        IMPORTANT        !
@echo  !                         !
@echo  ===========================
@echo.
@echo  Steps:
@echo  1. Set your ameba to UART Download mode using the buttons on the board
@echo  2. Check your ameba's serial port and key in the COM port (e.g.: COM8)
@echo.
@set /p port=Ameba COM port:
amebad_image_tool.exe %port%
@echo.
@echo You can close this window now
@pause
