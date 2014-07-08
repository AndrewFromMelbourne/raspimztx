OBJS=sysinfo.o ../common/lcd.o ../common/image.o ../common/key.o \
     ../common/font.o
BIN=sysinfo

CFLAGS+=-Wall -g -O3 -I../common
LDFLAGS+=-lbcm2835

all: $(BIN)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
