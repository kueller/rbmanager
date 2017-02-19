# RBManager

![alt text](http://i.imgur.com/NfgaNuI.png "RBManager")

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
qmake-qt5
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
qmake-qt5
make
make clean
```

## Usage

Click the "Open Drive" button and select your USB drive wth customs to load the song listing. Program will likely need to be run as root to do this.

Use File->Import to add your new customs to the drive. The delete key will remove any selected customs from the listing.
