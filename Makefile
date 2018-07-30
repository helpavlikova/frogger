CC = g++

# Linux (default)
TARGET = ass3
CFLAGS = -Wall -I.
LDFLAGS = -std=c++11 -lGL -lGLU -lglut ./libSOIL.a -lm

# Windows (cygwin)
ifeq "$(OS)" "Windows_NT"
	TARGET = ass3.exe
	CFLAGS += -D_WIN32
endif

# OS X
ifeq "$(OSTYPE)" "darwin"
	LDFLAGS = -framework Carbon -framework QuickTime -framework OpenGL -framework GLUT
	CFLAGS = -D__APPLE__ -Wall
endif


SRC = ass3.c
OBJ = ass3.o


all: $(TARGET)

debug: CFLAGS += -g
debug: all

$(TARGET): $(OBJ)
	@echo linking $(TARGET)
	@$(CC) -o $(TARGET) $(OBJ) $(CFLAGS) $(LDFLAGS)

%.o: %.c
	@echo compiling $@ $(CFLAGS)
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY:
clean:
	@echo cleaning $(OBJ)
	@rm $(OBJ)
