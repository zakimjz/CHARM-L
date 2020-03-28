CC = g++ 
CFLAGS = -O3 -Wno-deprecated
HDRS = eclat.h timetrack.h calcdb.h eqclass.h stats.h\
	maximal.h chashtable.h lattice.h constraints.h
OBJS = calcdb.o eqclass.o stats.o maximal.o eclat.o enumerate.o\
	chashtable.o lattice.o rules.o constraints.o
LIBS = 
TARGET = charm

default: $(TARGET)

clean:
	rm -rf *~ *.o $(TARGET)

charm: $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o charm $(OBJS) $(LIBS)
#	strip $(TARGET)

.SUFFIXES: .o .cpp

.cpp.o:
	$(CC) $(CFLAGS) -c $<


# dependencies
# $(OBJS): $(HDRS)
eclat.o: $(HDRS)
enumerate.o: $(HDRS)
calcdb.o: calcdb.h eclat.h eqclass.h
eqclass.o: eqclass.h eclat.h calcdb.h
maximal.o: maximal.h calcdb.h eqclass.h
stats.o: stats.h
chashtable.o: chashtable.h eclat.h
lattice.o: lattice.h
rules.o:
constraints.o:
