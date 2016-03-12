# flashmap
Automatically exported from code.google.com/p/flashmap

Added the pending pull request over at the main github site since noone else seems to bother.

# prerequisites (debian-based distros)

    apt-get install checkinstall -y

# install (debian)

    make clean && make && checkinstall
    
# uninstall (debian)

    dpkg -r flashmap
