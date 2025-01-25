CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
PREFIX = /usr
BINDIR = $(PREFIX)/bin
BUILDDIR = build

# Targets
all: prepare build

prepare:
	@mkdir -p $(BUILDDIR)
	@echo "Build directory prepared"

build: khazarfetch.cpp
	@echo "Building khazarfetch..."
	@$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/khazarfetch khazarfetch.cpp
	@echo "Build completed"

install: all
	@echo "Installing..."
	@install -d $(DESTDIR)$(BINDIR)
	@install -m 755 $(BUILDDIR)/khazarfetch $(DESTDIR)$(BINDIR)/khazarfetch
	@echo "Installation completed!"

uninstall:
	@echo "Uninstalling..."
	@rm -f $(DESTDIR)$(BINDIR)/khazarfetch
	@echo "Uninstall completed!"

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILDDIR)
	@echo "Clean completed!"

.PHONY: all prepare build install uninstall clean 