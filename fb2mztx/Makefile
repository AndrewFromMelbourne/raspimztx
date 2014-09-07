OBJS=fb2mztx.o ../common/lcd.o ../common/syslogUtilities.o \
     ../common/nearestNeighbour.o
BIN=fb2mztx

CFLAGS+=-Wall -g -O3 -I../common
LDFLAGS+=-lbcm2835 -lbsd

all: $(BIN)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
