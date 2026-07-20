CC := gcc
BEAR := bear

CFLAGS := -Wall -Wextra

GTK4_CFLAGS := $(shell pkg-config --cflags gtk4)
GTK4_LIBS := $(shell pkg-config --libs gtk4)

TRAY_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 ayatana-appindicator3-0.1)
TRAY_LIBS := $(shell pkg-config --libs gtk+-3.0 ayatana-appindicator3-0.1)

TARGET := brightness
TRAY_TARGET := brightness-tray

APP_ID := com.brightness.control

PREFIX := $(HOME)/.local
BIN_DIR := $(PREFIX)/bin
APPLICATION_DIR := $(PREFIX)/share/applications
ICON_THEME_DIR := $(PREFIX)/share/icons/hicolor
ICON_DIR := $(ICON_THEME_DIR)/512x512/apps
AUTOSTART_DIR := $(HOME)/.config/autostart

DESKTOP_FILE := $(APPLICATION_DIR)/$(APP_ID).desktop
AUTOSTART_FILE := $(AUTOSTART_DIR)/$(APP_ID).tray.desktop

all: bear

bear:
	rm -f compile_commands.json
	$(MAKE) clean
	$(BEAR) -- $(MAKE) build

build: $(TARGET) $(TRAY_TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) $(GTK4_CFLAGS) \
		main.c \
		-o $(TARGET) \
		$(GTK4_LIBS)

$(TRAY_TARGET): tray.c
	$(CC) $(CFLAGS) $(TRAY_CFLAGS) \
		tray.c \
		-o $(TRAY_TARGET) \
		$(TRAY_LIBS)

install: build
	mkdir -p $(BIN_DIR)
	mkdir -p $(APPLICATION_DIR)
	mkdir -p $(ICON_DIR)
	mkdir -p $(AUTOSTART_DIR)

	cp $(TARGET) $(BIN_DIR)/$(TARGET)
	cp $(TRAY_TARGET) $(BIN_DIR)/$(TRAY_TARGET)

	cp icons/512.png \
		$(ICON_DIR)/$(APP_ID).png

	printf '%s\n' \
		'[Desktop Entry]' \
		'Version=1.0' \
		'Type=Application' \
		'Name=Brightness Control' \
		'Comment=Control external monitor brightness via DDC/CI' \
		'Exec=$(BIN_DIR)/$(TARGET)' \
		'Icon=$(APP_ID)' \
		'Terminal=false' \
		'Categories=Utility;Settings;' \
		'StartupNotify=true' \
		> $(DESKTOP_FILE)

	printf '%s\n' \
		'[Desktop Entry]' \
		'Version=1.0' \
		'Type=Application' \
		'Name=Brightness Control Tray' \
		'Comment=Monitor brightness top-panel indicator' \
		'Exec=$(BIN_DIR)/$(TRAY_TARGET)' \
		'Icon=$(APP_ID)' \
		'Terminal=false' \
		'NoDisplay=true' \
		'X-GNOME-Autostart-enabled=true' \
		> $(AUTOSTART_FILE)

	chmod 755 $(BIN_DIR)/$(TARGET)
	chmod 755 $(BIN_DIR)/$(TRAY_TARGET)
	chmod 644 $(DESKTOP_FILE)
	chmod 644 $(AUTOSTART_FILE)
	chmod 644 $(ICON_DIR)/$(APP_ID).png

	gtk4-update-icon-cache -f -t \
		$(ICON_THEME_DIR) 2>/dev/null || true

	update-desktop-database \
		$(APPLICATION_DIR) 2>/dev/null || true

	@echo "Installed."
	@echo "Application: $(BIN_DIR)/$(TARGET)"
	@echo "Tray:        $(BIN_DIR)/$(TRAY_TARGET)"
	@echo "Start tray now with:"
	@echo "  $(BIN_DIR)/$(TRAY_TARGET)"

uninstall:
	pkill -x $(TRAY_TARGET) 2>/dev/null || true

	rm -f $(BIN_DIR)/$(TARGET)
	rm -f $(BIN_DIR)/$(TRAY_TARGET)
	rm -f $(DESKTOP_FILE)
	rm -f $(AUTOSTART_FILE)
	rm -f $(ICON_DIR)/$(APP_ID).png

	gtk4-update-icon-cache -f -t \
		$(ICON_THEME_DIR) 2>/dev/null || true

	update-desktop-database \
		$(APPLICATION_DIR) 2>/dev/null || true

	@echo "Uninstalled."

clean:
	rm -f $(TARGET)
	rm -f $(TRAY_TARGET)

fclean: clean
	rm -f compile_commands.json

re:
	$(MAKE) fclean
	$(MAKE) all

.PHONY: all bear build install uninstall clean fclean re
