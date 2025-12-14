CC = gcc
CFLAGS = -Wall -fPIC
LDFLAGS = -shared
DLFLAGS = -ldl

all: libs static_prog dynamic_prog

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $< -o $@

libpi_leibniz.so: pi_leibniz.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

libpi_wallis.so: pi_wallis.o
	$(CC) $(LDFLAGS) -o $@ $< -lm

libconvert_binary.so: convert_binary.o
	$(CC) $(LDFLAGS) -o $@ $<

libconvert_ternary.so: convert_ternary.o
	$(CC) $(LDFLAGS) -o $@ $<

static_prog: main_static.o utils.o
	$(CC) -o $@ $^ -L. -lpi_leibniz -lconvert_binary -Wl,-rpath,.

main_static.o: main_static.c utils.h contracts.h
	$(CC) $(CFLAGS) -c $< -o $@

dynamic_prog: main_dynamic.o utils.o
	$(CC) -o $@ $^ $(DLFLAGS)

main_dynamic.o: main_dynamic.c utils.h
	$(CC) $(CFLAGS) -c $< -o $@

pi_leibniz.o: pi_leibniz.c contracts.h
	$(CC) $(CFLAGS) -c $< -o $@

pi_wallis.o: pi_wallis.c contracts.h
	$(CC) $(CFLAGS) -c $< -o $@

convert_binary.o: convert_binary.c contracts.h
	$(CC) $(CFLAGS) -c $< -o $@

convert_ternary.o: convert_ternary.c contracts.h
	$(CC) $(CFLAGS) -c $< -o $@

libs: libpi_leibniz.so libpi_wallis.so libconvert_binary.so libconvert_ternary.so

clean:
	rm -f *.o *.so static_prog dynamic_prog

.PHONY: all libs clean