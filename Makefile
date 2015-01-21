SERVICES=datetime.c echo.c
SERVICES:=$(addprefix services/, $(SERVICES))
# USER_INCLUDE+=-I../include
# USER_LIB+=-L../lib

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)gcc

CFLAGS+=$(OPTION) $(USER_CFLAGS) $(USER_INCLUDE) $(DEBUG) -I$(PWD) -g -Wall # -Werror
LDFLAGS+=$(OPTION) $(USER_LDFLAGS) $(USER_LIB) -levent

SOURCES=main.c evops.c services.c $(SERVICES)
OBJS=$(SOURCES:.c=.o)
TARGET=event-test

all:$(TARGET)

$(TARGET):$(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

test-%:%.c
	$(CC) -DTEST $^ $(LDFLAGS) -o $@

services.c:services.py $(SERVICES)
	./$^ > $@

# dependencies
include $(SOURCES:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) $(CFLAGS) -MM $< -MT $*.o > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY:all clean

clean:
	-rm $(TARGET) $(OBJS) services.c

dep-clean:
	-(find . -name '*.d' -or -name '*.d.*' -or -name '*~' | xargs rm)
