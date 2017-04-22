CC = clang
LIBS = -lSDL2 -lSDL2_ttf -lm
CFLAGS = -Wall -std=c99
LDFLAGS = $(LIB)
TARGET = pong

SRCDIR = src
BUILDDIR = build

.PHONY: default all clean run

default: $(TARGET)
all: default

OBJECTS = $(patsubst $(SRCDIR)/%, $(BUILDDIR)/%, $(patsubst %.c, %.o, $(wildcard $(SRCDIR)/*.c)))
HEADERS = $(patsubst $(SRCDIR)/%, $(BUILDDIR)/%, $(wildcard $(SRCDIR)/*.h))

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	-mkdir $(BUILDDIR)

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -rf $(BUILDDIR)
	-rm -f $(TARGET)

run: $(TARGET)
	@./$(TARGET) || true
