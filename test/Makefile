OBJS=test.o ../common/lcd.o ../common/image.o
BIN=test

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
