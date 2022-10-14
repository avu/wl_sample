sudo apt-get install -y wayland-protocols libgles2-mesa-dev
wayland-scanner private-code   < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml   > xdg-shell-protocol.c
wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-client-protocol.h
