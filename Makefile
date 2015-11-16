TARGETS=	dmx2mztx \
			fb2mztx \
			jpg2mztx \
			png2mztx \
			raspinfo \
			test \
			webcam

default :all

all:
	for target in $(TARGETS); do ($(MAKE) -C $$target); done

clean:
	for target in $(TARGETS); do ($(MAKE) -C $$target clean); done

