CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
RECOMMENDS = nerd-fonts
PREFIX = /usr
BINDIR = $(PREFIX)/bin
SRCDIR = src
BUILDDIR = build
TARGET = khazarfetch

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
	@echo "Build completed"

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

install: $(TARGET)
	@echo "Installing..."
	@install -d $(DESTDIR)$(BINDIR)
	@install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "Installation completed!"

uninstall:
	@echo "Uninstalling..."
	@rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "Uninstall completed!"

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILDDIR) $(TARGET)
	@echo "Clean completed!"

.PHONY: all install uninstall clean
