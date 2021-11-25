# This Makefile was edited by EXL, 25-Nov-2021

MOTOMAGX_DEVICE_PATH      = /arm-eabi
MOTOMAGX_DEVICE_CC        = $(MOTOMAGX_DEVICE_PATH)/bin/arm-linux-gnueabi-gcc
MOTOMAGX_DEVICE_CXX       = $(MOTOMAGX_DEVICE_PATH)/bin/arm-linux-gnueabi-g++
MOTOMAGX_DEVICE_STRIP     = $(MOTOMAGX_DEVICE_PATH)/bin/arm-linux-gnueabi-strip
MOTOMAGX_DEVICE_CFLAGS    = -pipe -Wall -W -O2
MOTOMAGX_DEVICE_CXXFLAGS  = -pipe -DQWS -fno-exceptions -fno-rtti -Wall -W -O2

MOTOMAGX_EMULATOR_PATH      = /opt/toolchains/motomagx/emulator
MOTOMAGX_EMULATOR_CC        = $(MOTOMAGX_EMULATOR_PATH)/bin/i686-mot-linux-gnu-gcc
MOTOMAGX_EMULATOR_CXX       = $(MOTOMAGX_EMULATOR_PATH)/bin/i686-mot-linux-gnu-g++
MOTOMAGX_EMULATOR_STRIP     = $(MOTOMAGX_EMULATOR_PATH)/bin/i686-mot-linux-gnu-strip
MOTOMAGX_EMULATOR_CFLAGS    = -pipe -Wall -W -O2
MOTOMAGX_EMULATOR_CXXFLAGS  = -pipe -DQWS -fno-exceptions -fno-rtti -Wall -W -O2

all: emulator device

emulator: fbgrab_EMU jgrab_EMU dgrab_EMU zgrab_EMU
device: fbgrab jgrab dgrab zgrab

fbgrab_EMU: fbgrab.c
	$(MOTOMAGX_EMULATOR_CC) $(MOTOMAGX_EMULATOR_CFLAGS) \
		fbgrab.c -o fbgrab_EMU
	$(MOTOMAGX_EMULATOR_STRIP) -s fbgrab_EMU

fbgrab: fbgrab.c
	$(MOTOMAGX_DEVICE_CC) $(MOTOMAGX_DEVICE_CFLAGS) \
		fbgrab.c -o fbgrab
	$(MOTOMAGX_DEVICE_STRIP) -s fbgrab

jgrab_EMU: jgrab.c
	$(MOTOMAGX_EMULATOR_CC) $(MOTOMAGX_EMULATOR_CFLAGS) \
		-I$(MOTOMAGX_EMULATOR_PATH)/include \
		jgrab.c -o jgrab_EMU \
		-L$(MOTOMAGX_EMULATOR_PATH)/lib -ljpeg
	$(MOTOMAGX_EMULATOR_STRIP) -s jgrab_EMU

jgrab: jgrab.c
	$(MOTOMAGX_DEVICE_CC) $(MOTOMAGX_DEVICE_CFLAGS) \
		-I$(MOTOMAGX_DEVICE_PATH)/arm-linux-gnueabi/include \
		jgrab.c -o jgrab \
		-L$(MOTOMAGX_DEVICE_PATH)/arm-linux-gnueabi/lib -ljpeg
	$(MOTOMAGX_DEVICE_STRIP) -s jgrab

zgrab_EMU: zgrab.cpp
	$(MOTOMAGX_EMULATOR_CXX) $(MOTOMAGX_EMULATOR_CXXFLAGS) \
		-I$(MOTOMAGX_EMULATOR_PATH)/include \
		zgrab.cpp -o zgrab_EMU \
		-L$(MOTOMAGX_EMULATOR_PATH)/lib -lqte-mt
	$(MOTOMAGX_EMULATOR_STRIP) -s zgrab_EMU

zgrab: zgrab.cpp
	$(MOTOMAGX_DEVICE_CXX) $(MOTOMAGX_DEVICE_CXXFLAGS) \
		-I$(MOTOMAGX_DEVICE_PATH)/lib/qt-zn5/include \
		-I$(MOTOMAGX_DEVICE_PATH)/lib/ezx-zn5/include \
		-I$(MOTOMAGX_DEVICE_PATH)/arm-linux-gnueabi/include \
		zgrab.cpp -o zgrab \
		-Wl,-rpath-link,$(MOTOMAGX_DEVICE_PATH)/lib/ezx-zn5/lib \
		-L$(MOTOMAGX_DEVICE_PATH)/lib/ezx-zn5/lib -lqte-mt
	$(MOTOMAGX_DEVICE_STRIP) -s zgrab

dgrab_EMU: dgrab.cpp
	$(MOTOMAGX_EMULATOR_CXX) $(MOTOMAGX_EMULATOR_CXXFLAGS) \
		-I$(MOTOMAGX_EMULATOR_PATH)/include \
		dgrab.cpp -o dgrab_EMU \
		-L$(MOTOMAGX_EMULATOR_PATH)/lib -lqte-mt
	$(MOTOMAGX_EMULATOR_STRIP) -s dgrab_EMU

dgrab: dgrab.cpp
	$(MOTOMAGX_DEVICE_CXX) $(MOTOMAGX_DEVICE_CXXFLAGS) \
		-I$(MOTOMAGX_DEVICE_PATH)/lib/qt-zn5/include \
		-I$(MOTOMAGX_DEVICE_PATH)/lib/ezx-zn5/include \
		-I$(MOTOMAGX_DEVICE_PATH)/arm-linux-gnueabi/include \
		dgrab.cpp -o dgrab \
		-Wl,-rpath-link,$(MOTOMAGX_DEVICE_PATH)/lib/ezx-zn5/lib \
		-L$(MOTOMAGX_DEVICE_PATH)/lib/ezx-zn5/lib -lqte-mt
	$(MOTOMAGX_DEVICE_STRIP) -s dgrab

clean:
	-rm -f fbgrab jgrab dgrab zgrab
	-rm -f fbgrab_EMU jgrab_EMU dgrab_EMU zgrab_EMU
	-rm -f MagxScreenshot.zip

zip: all
	-zip -r -9 MagxScreenshot.zip \
		fbgrab.c jgrab.c dgrab.cpp zgrab.cpp \
		fbgrab jgrab dgrab zgrab \
		fbgrab_EMU jgrab_EMU dgrab_EMU zgrab_EMU
