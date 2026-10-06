# Makefile, by Zhong Jun

########## cross define ######################
CROSS=~/Inlock/pc/toolchain/mips-gcc540-glibc222-r3.3.7.mxu2.cve2/bin/mips-linux-gnu-
CC        = $(CROSS)gcc
CXX       = $(CROSS)g++
LD        = $(CROSS)ld
AR        = $(CROSS)ar
STRIP     = $(CROSS)strip

$(warning CROSS=$(CROSS))
########## source files #####################

CSRC += $(wildcard *.c)
OBJS += $(patsubst %.c, %.o, $(CSRC))

TARGET = ovd

########## compile and link flags ###########
CFLAGS += -I.
CFLAGS += -I../include

LIBS +=
# Debug Options
#ifdef RELEASE
  CXXFLAGS += -muclibc -O2
  CFLAGS += -muclibc -O2
#else
#  CXXFLAGS += -g -O0
#  CFLAGS += -g -O0
#endif


LDFLAGS  += -L../lib -lovdsdk  -lcjson -lmbedtls -lmbedx509 -lmbedcrypto -lpthread -latomic

########## utils define ######################
BUILD_MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi


all:$(TARGET)

$(TARGET):$(OBJS)
	@$(BUILD_MKDIR)
	$(CXX) -o $@ $^ $(STATIC_LIB) $(LDFLAGS)
	$(STRIP) $(TARGET)
	-rm -f $(OBJS)


%.o : %.c
	@$(BUILD_MKDIR)
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.cpp
	@$(BUILD_MKDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJS)
	-rm -f $(TARGET)

.PHONY:clean all exe
