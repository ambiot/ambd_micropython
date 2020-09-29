.PHONY: all

OS := $(shell uname)

all:
ifeq ($(findstring CYGWIN, $(OS)), CYGWIN)
	mkdir -p windows
	tar -xvf postbuildTool_windows.tar -C windows/
else
	mkdir -p linux
	tar -xvf postbuildTool_linux.tar -C linux/
endif