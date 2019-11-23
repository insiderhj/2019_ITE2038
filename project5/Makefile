.SUFFIXES: .cpp .o

CC=g++

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.cpp)
# OBJS:=$(SRCS:.cpp=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)bpt.cpp $(SRCDIR)disk_space_manager.cpp $(SRCDIR)buffer_manager.cpp $(SRCDIR)printer.cpp $(SRCDIR)join.cpp $(SRCDIR)lock_manager.cpp $(SRCDIR)util.cpp
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)

CFLAGS+= -std=c++14 -g -fPIC -I $(INC) -Wall -Wformat=0

TARGET=main

all: $(TARGET)

$(SRCDIR)bpt.o:
	$(CC) $(CFLAGS) -o $(SRCDIR)bpt.o -c $(SRCDIR)bpt.cpp

$(SRCDIR)disk_space_manager.o:
	$(CC) $(CFLAGS) -o $(SRCDIR)disk_space_manager.o -c $(SRCDIR)disk_space_manager.cpp

$(SRCDIR)buffer_manager.o:
	$(CC) $(CFLAGS) -o $(SRCDIR)buffer_manager.o -c $(SRCDIR)buffer_manager.cpp

$(SRCDIR)printer.o:
	$(CC) $(CFLAGS) -o $(SRCDIR)printer.o -c $(SRCDIR)printer.cpp

$(SRCDIR)join.o:
	$(CC) $(CFLAGS) -o $(SRCDIR)join.o -c $(SRCDIR)join.cpp

$(SRCDIR)lock_manager.o:
	$(CC) $(CFLAGS) -o $(SRCDIR)lock_manager.o -c $(SRCDIR)lock_manager.cpp

$(SRCDIR)util.o:
	$(CC) $(CFLAGS) -o $(SRCDIR)util.o -c $(SRCDIR)util.cpp

$(TARGET_OBJ):
	$(CC) $(CFLAGS) -o $(TARGET_OBJ) -c $(TARGET_SRC)

$(TARGET): $(OBJS_FOR_LIB) $(TARGET_OBJ)
	make static_library
	$(CC) $(CFLAGS) -o $@ $^ -L $(LIBS) -lbpt

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*

library:
	g++ -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	mkdir -p $(LIBS)
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)
