TARGETS=	dmx2mztx \
			fb2mztx \
			jpg2mztx \
			png2mztx \
			raspinfo \
			test

default :all

all:
	for target in $(TARGETS); do (make -C $$target); done

clean:
	for target in $(TARGETS); do (make -C $$target clean); done

