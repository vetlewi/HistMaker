# OCLDAQ [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.1206106.svg)](https://doi.org/10.5281/zenodo.1206106)
The OCL DAQ software is a package of programs that are used for readout of data aquisition systems for nuclear physics experiments.
The original package contains an event builder to be ran on a dedicated Lynx computer in a VME crate. Newer verions will transition to a XIA based system.

## TODO list
* Create a wrapper class to wrap all XIA library functions such that it can be thread-safe
* De-couple the data buffer from the XIA interface
* Write the scalers as CSV

Fix of PLX drivers
---

The XIA library for the Pixie-16 relies on the PLX drivers 
for communication with the modules. The linux library are however
not updated and uses a fairly old version from 2013. This version
does not support linux kernels newer than 3.X, which is problematic
since support of linux 3.X ends in 2020.

Luckily the drivers can be modified to compile on newer kernels, though
they have not been tested yet.

Here is a list of patches that has to be done:
1) Change include header from `#include <asm/uaccess.h>` to
`#include <linux/uaccess.h>` in `ApiFunc.c` and `Dispatcher.c`
(read [here](https://medium.com/@avenger.v14/hi-when-building-the-enhanced-example-with-character-device-i-encountered-a-build-error-for-55079354f704)) 
2) Comment out line `860, 861` and `865` in `SuppFunc.c`
3) If kernel version >= 5, comment out line `1905` and `1918` in `ApiFunc.c`.
