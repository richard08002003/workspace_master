ifeq ($(I_intel),1)   
CC := /opt/clanton-tiny/1.4.2/sysroots/i686-pokysdk-linux/usr/bin/i586-poky-linux-uclibc/i586-poky-linux-uclibc-gcc
SYSROOT_PATH := /home/richard/UBike2_ToolChain/ToolChain/openssl/MP-1501C/clanton 
SYSROOT := --sysroot=${SYSROOT_PATH}
else ifeq ($(I_arm),1)
CC := /media/richard/richard_hdd/RaspberryPi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-gcc
SYSROOT_PATH:=/media/richard/richard_hdd/RaspberryPi/tools
SYSROOT=--sysroot=${SYSROOT_PATH}
else
CC :=gcc
endif
LDFLAGS := ${SYSROOT} -lssl -lcrypto -lpthread -lbluetooth

OBJECTS:= Common_Tool.c Crypto.c Epoll.c File.c Ftp.c Http.c IPC.c Json.c Linked_list.c MyLog.c RS232.c Tcp.c DHT.c Ble.c Gpio.c
#OBJECTS:= *.c 
Define= D_NULL

CFLAGS:= -s -g -Wall -Wno-write-strings

all:	
	$(CC) -c $(OBJECTS) $(CFLAGS)  $(LDFLAGS) -D $(Define)
	mkdir -p ./obj
	mv -f ./*.o ./obj
clean:
	rm -r ./obj &
#	rm -r ./*.o &
