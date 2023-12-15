# Development
This guide will help you build and install QtAliceVision plugin.

## Requirements
QtAliceVision requires:
* [Qt5](https://www.qt.io/) (== 5.15.2, make sure to use the **same version** as the target application)
* [AliceVision](https://github.com/aliceVision/AliceVision)
* [OpenImageIO](https://github.com/AcademySoftwareFoundation/OpenImageIO) (>= 2.1) - with OpenEXR support for depthmaps visualization
* [Alembic](https://github.com/alembic/alembic) (>= 1.7)
* [CMake](https://cmake.org/) (>= 3.11)
* On Windows platform: Microsoft Visual Studio (>= 2019)

> **Note for Windows**:
We recommend using [VCPKG](https://github.com/Microsoft/vcpkg) to get Alembic. Qt can either be installed with VCPKG or the official installer.


## Build instructions

In the following steps, replace <INSTALL_PATH> with the installation path of your choice.


#### Windows
> We will use "NMake Makefiles" generators here to have one-line build/installation,
but Visual Studio solutions can be generated if need be.

From a developer command-line, using Qt 5.15.2 (built with VS2019) and VCPKG for OIIO:
```
set QT_DIR=/path/to/qt/5.15.2/msvc2019_64
set AV_DIR=/path/to/aliceVision/cmake/folder
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DAliceVision_DIR=%AV_DIR% -DCMAKE_PREFIX_PATH=%QT_DIR% -DVCPKG_TARGET_TRIPLET=x64-windows -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> 
nmake install
```

#### Linux

```bash
export QT_DIR=/path/to/qt/5.15.2/gcc_64
export OPENIMAGEIO_DIR=/path/to/oiio/install
export ALEMBIC_DIR=/path/to/alembic/config
cmake .. -DAliceVision_DIR=$AV_DIR -DOPENIMAGEIO_LIBRARY_DIR_HINTS:PATH=$OPENIMAGEIO_DIR/lib/ -DOPENIMAGEIO_INCLUDE_DIR:PATH=$OPENIMAGEIO_DIR/include/ -DAlembic_DIR=$ALEMBIC_DIR -DCMAKE_PREFIX_PATH=$QT_DIR -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> -DCMAKE_BUILD_TYPE=Release
make install
```

## Usage
Once built, setup those environment variables before launching your application:

```bash
# Windows:
set QML2_IMPORT_PATH=<INSTALL_PATH>/qml;%QML2_IMPORT_PATH%
set QT_PLUGIN_PATH=<INSTALL_PATH>;%QT_PLUGIN_PATH%

# Linux:
export QML2_IMPORT_PATH=<INSTALL_PATH>/qml:$QML2_IMPORT_PATH
export QT_PLUGIN_PATH=<INSTALL_PATH>:$QT_PLUGIN_PATH
```
