OBJS=jpg2mztx.o ../common/lcd.o ../common/image.o ../common/key.o \
     ../common/nearestNeighbour.o
BIN=jpg2mztx

CFLAGS+=-Wall -g -O3 -I../common
LDFLAGS+=-lbcm2835 -ljpeg

all: $(BIN)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
