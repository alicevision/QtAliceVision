# Development
This guide will help you build and install QtAliceVision plugin.

## Requirements
QtAliceVision requires:
* [Qt5](https://www.qt.io/) (>= 5.10, make sure to use the **same version** as the target application)
* [AliceVision](https://github.com/aliceVision/AliceVision)
* [CMake](https://cmake.org/) (>= 3.3)
* On Windows platform: Microsoft Visual Studio (>= 2015.3)


## Build instructions

In the following steps, replace <INSTALL_PATH> with the installation path of your choice.


#### Windows
> We will use "NMake Makefiles" generators here to have one-line build/installation,
but Visual Studio solutions can be generated if need be.

From a developer command-line, using Qt 5.12 (built with VS2015):
```
set QT_DIR=/path/to/qt/5.12.0/msvc2015_64
set AV_DIR=/path/to/aliceVision/cmake/folder
cmake .. -DAliceVision_DIR=%AV_DIR% -DCMAKE_PREFIX_PATH=%QT_DIR% -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> 
nmake install
```

#### Linux

```bash
export QT_DIR=/path/to/qt/5.12.0/gcc_64
cmake .. -DAliceVision_DIR=$AV_DIR -DCMAKE_PREFIX_PATH=$QT_DIR -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> -DCMAKE_BUILD_TYPE=Release
make install
```

## Usage
Once built, add the install folder of this plugin to the `QML2_IMPORT_PATH` before launching your application:

```bash
# Windows:
set QML2_IMPORT_PATH=<INSTALL_PATH>/qml;%QML2_IMPORT_PATH%

# Linux:
export QML2_IMPORT_PATH=<INSTALL_PATH>/qml:$QML2_IMPORT_PATH
```
