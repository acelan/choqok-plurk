Plurk plugin for Choqok
=======================
This is a [Choqok] (http://choqok.gnufolks.org/) plugin for [Plurk] (http://www.plurk.com/). 

Screenshots
-----------
![screenshot1](https://github.com/acelan/choqok-plurk/raw/master/doc/images/screenshot1.png)
![screenshot2](https://github.com/acelan/choqok-plurk/raw/master/doc/images/screenshot2.png)


Installation
------------
### Requirements to build ###
+ CMake 2.6
+ KDE 4 library Development packages! (On (K)Ubuntu/Debian it's kdelibs5-dev)
+ QJSon library (Development packages)
+ QOAuth library (Development packages)
+ QCA2 library (Development packages)
+ Choqok

### How To Build The Project ###
	cd choqok-plurk
	mkdir build
	cd build
	cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` ..
	make
	sudo make install OR su -c 'make install'

### Uninstall the project ###
	make uninstall or su -c 'make uninstall'

Developed By
-----------
* AceLan Kao <acelan@acelan.idv.tw> - [http://blog.acelan.idv.tw] (http://blog.acelan.idv.tw)
* Wei Cheng Pan <legnaleurc@gmail.com>
* Franklin Weng <franklin@goodhorse.idv.tw>

License
-------
Licensed under the GNU General Public License(GPL), version 2.
