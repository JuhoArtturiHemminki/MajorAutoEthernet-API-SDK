CC = gcc
CFLAGS = -O3 -march=native -ffast-math -Wall

ifeq ($(OS),Windows_NT)
    TARGET = build/major_auto_ethernet.dll
    LDFLAGS = -shared -Wl,--export-all-symbols
    RM = rmdir /s /q
    MKDIR = mkdir
    FIXPATH = $(subst /,\,$1)
else
    TARGET = build/major_auto_ethernet.so
    LDFLAGS = -shared -fPIC
    RM = rm -rf
    MKDIR = mkdir -p
    FIXPATH = $1
endif

.PHONY: all clean test

all: $(TARGET)

$(TARGET): src/major_auto_ethernet.c
	@if not exist build $(MKDIR) build 2>nul || true
	$(CC) $(CFLAGS) $(LDFLAGS) src/major_auto_ethernet.c -o $(TARGET)

test: $(TARGET)
	python bindings/mae_wrapper.py

clean:
	-$(RM) $(call FIXPATH,build)
