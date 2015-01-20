USER_SRCS+=echo.c
# USER_INCLUDE+=-I../include
# USER_LIB+=-L../lib

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)gcc

CFLAGS+=$(OPTION) $(USER_CFLAGS) $(USER_INCLUDE) $(DEBUG) -g -Wall # -Werror
LDFLAGS+=$(OPTION) $(USER_LDFLAGS) $(USER_LIB) -levent

SOURCES=main.c evops.c $(USER_SRCS)
OBJS=$(SOURCES:.c=.o)
TARGET=event-test

all:$(TARGET)

$(TARGET):$(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

test-%:%.c
	$(CC) -DTEST $^ $(LDFLAGS) -o $@

# dependencies
include $(SOURCES:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) $(CFLAGS) -MM $< -MT $*.o > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY:all clean

clean:
	-rm $(TARGET) $(OBJS)

dep-clean:
	-rm *.d *.d.*
