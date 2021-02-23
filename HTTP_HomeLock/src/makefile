Tool_obj_path=/media/richard/richard_hdd/workspace_NCHU/Common_tool_C/src/obj

ifeq ($(I_intel),1)   
CC=/opt/clanton-tiny/1.4.2/sysroots/i686-pokysdk-linux/usr/bin/i586-poky-linux-uclibc/i586-poky-linux-uclibc-gcc
SYSROOT_PATH:=/home/richard/UBike2_ToolChain/ToolChain/openssl/MP-1501C/clanton 
SYSROOT=--sysroot=${SYSROOT_PATH}
else
CC:=gcc
endif

ifeq ($(I_mqtt),1)
Define=D_mqtt
LDFLAGS=${SYSROOT} -lssl -lcrypto  -lpthread -lpaho-mqtt3cs -lbluetooth
else
Define=D_NULL
LDFLAGS=${SYSROOT} -lssl -lcrypto  -lpthread -lbluetooth
endif

CFLAGS:= -s -g -Wall  -Wno-write-strings
#CFLAGS:= -s -Wall -Wno-write-strings -std=c++11 -D_GLIBCXX_USE_C99

BIN:= Http_HomeLock


all:
	mkdir -p ./tmp 
	 
	cp -a $(Tool_obj_path)/* ./tmp
	
	cp -a ./*.h ./*.c ./tmp
	
	$(CC) -c ./tmp/*.c $(CFLAGS) $(LDFLAGS) -D $(Define)
	mv -f ./*.o ./tmp

	$(CC) ./tmp/*.o  -o $(BIN) $(LDFLAGS)  $(CFLAGS)  -D $(Define)
	
clean:
	rm -r ./tmp	&
	rm -r ./*.o &
	rm -r ./$(BIN) &
