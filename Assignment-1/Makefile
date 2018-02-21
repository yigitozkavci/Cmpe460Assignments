COMPILER      = g++
OPTIONS       = -std=c++14 -o
LINKER_OPT    = -L/usr/lib -lm


all: main

main: main.cpp bitmap_image.hpp
	$(COMPILER) $(OPTIONS) main main.cpp $(LINKER_OPT)

clean:
	rm -f core *.o *.bak *stackdump *~

#
# The End !
#
