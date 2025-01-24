CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
TARGET = khazarfetch
PREFIX = /usr

all: $(TARGET)

$(TARGET): khazarfetch.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) khazarfetch.cpp
	@echo "Derleme tamamlandı, /usr/bin dizinine kopyalanıyor..."
	@sudo cp $(TARGET) $(PREFIX)/bin/$(TARGET)
	@sudo chmod 755 $(PREFIX)/bin/$(TARGET)
	@echo "Kurulum tamamlandı!"

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	sudo rm -f $(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET)
	sudo rm -f $(PREFIX)/bin/$(TARGET)

.PHONY: all install uninstall clean 