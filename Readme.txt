Pacakages to be installed. For MSYS2 terminal.

# Core build tools
pacman -Syu        # First update everything
pacman -S git make pkg-config yasm nasm

# Optional but recommended for performance and debugging
pacman -S diffutils patch texinfo zlib

# Tools for working with build scripts
pacman -S autoconf automake libtool m4
