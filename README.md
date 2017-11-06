# RBManager

![alt text](https://i.imgur.com/8PsI5uu.png "RBManager")

## Installation

### Ubuntu

Dependencies:
```
qt5-qmake, qt5-default, build-essential
```

Build:
```
qmake src/
make
make clean
```

### Arch

Dependencies:
```
qt5-base, gcc, make
```

Build:
```
qmake-qt5 src/
make
make clean
```

### Fedora

Dependencies:
```
qt5-devel, make, gcc-c++
```

Build:
```
qmake-qt5 src/
make
make clean
```

## Usage

Click the "Open Drive" button and select your USB drive wth customs to load the song listing. Program will likely need to be run as root to do this.

Drag and drop files into the custom list to add them. The delete key will remove any selected customs in the listing from your drive.
