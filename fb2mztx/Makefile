OBJS=fb2mztx.o ../common/lcd.o ../common/syslogUtilities.o \
     ../common/resizeDispmanX.o
BIN=fb2mztx

CFLAGS+=-Wall -g -O3 -I../common
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host -lbcm2835 -lbsd

INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux

all: $(BIN)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
