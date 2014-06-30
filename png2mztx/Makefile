OBJS=png2mztx.o ../common/lcd.o ../common/image.o ../common/key.o \
     ../common/loadpng.o ../common/nearestNeighbour.o
BIN=png2mztx

CFLAGS+=-Wall -g -O3 -I../common $(shell libpng-config --cflags)
LDFLAGS+=-lbcm2835 $(shell libpng-config --ldflags)

all: $(BIN)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
