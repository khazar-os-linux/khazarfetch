CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
TARGET = khazarfetch

all: $(TARGET)

$(TARGET): khazarfetch.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) khazarfetch.cpp

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)/usr/bin/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all install uninstall clean 