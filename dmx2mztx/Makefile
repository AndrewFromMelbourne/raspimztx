OBJS=dmx2mztx.o ../common/lcd.o ../common/syslogUtilities.o
BIN=dmx2mztx

CFLAGS+=-Wall -g -O3 -I../common
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host -lbcm2835 -lbsd

INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux

all: $(BIN)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

$(BIN): $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
