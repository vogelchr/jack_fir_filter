CFLAGS=-Wall -Wextra -Os -ggdb
LDFLAGS=

JACK_LIBS=-ljack -lm
TEST_LIBS=-lm

JACK_OBJS = jack_multi_fir_filter.o fir_filter.o jack_interface.o 
TEST_OBJS = fir_filter.o fir_filter_test.o

ALL_OBJS = $(JACK_OBJS) $(TEST_OBJS)

all : jack_multi_fir_filter fir_filter_test

fir_filter_test : $(TEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(TEST_LIBS)

jack_multi_fir_filter : $(JACK_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(JACK_LIBS)

ifneq ($(MAKECMDGOALS), clean)
include $(ALL_OBJS:.o=.d)
endif

%.d : %.c
	$(CC) $(CPPFLAGS) -o $@ -MM $^

.PHONY : clean
clean :
	# build files
	rm -f *~ *.d *.o *.bak
	# applications
	rm -f jack_multi_fir_filter fir_filter_test
	# test files
	rm -f *.bin taps.txt
