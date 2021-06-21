# Development
This guide will help you build and install QtAliceVision plugins.

## Requirements
`QtAliceVision` requires at least:
* [CMake](https://cmake.org/) (>= 3.4)
* [AliceVision](https://github.com/aliceVision/AliceVision)
* [Qt5](https://www.qt.io/) (>= 5.13, make sure to use the **same version** as the target application)
* On Windows platform: Microsoft Visual Studio (>= 2015.3)

`qtAliceVision_imageIO_qtHandler` and `qtAliceVision_imageIO` plugins requires:
* [OpenImageIO](https://github.com/https://github.com/OpenImageIO/oiio) (>= 1.8.7) - with OpenEXR support for depthmaps visualization

`qmlAlembic` plugin requires:
* [Alembic](https://github.com/alembic/alembic) (>= 1.7)

> **Note for Windows**:
We recommend using [VCPKG](https://github.com/Microsoft/vcpkg) to get OpenImageIO / Alembic. Qt version there is too old at the moment though, using official installer is necessary.

## Build instructions

In the following steps, replace <INSTALL_PATH> with the installation path of your choice.

#### Windows
> We will use "NMake Makefiles" generators here to have one-line build/installation,
but Visual Studio solutions can be generated if need be.

From a developer command-line, using Qt 5.13 (built with VS2017) and VCPKG for OIIO:
```
set QT_DIR=/path/to/qt/5.13.0/msvc2017_64
set AV_DIR=/path/to/aliceVision/cmake/folder
cmake .. -DAliceVision_DIR=%AV_DIR% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DCMAKE_PREFIX_PATH=%QT_DIR% -DVCPKG_TARGET_TRIPLET=x64-windows -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH>
nmake install
```

#### Linux

```bash
export QT_DIR=/path/to/qt/5.13.0/gcc_64
export OPENIMAGEIO_DIR=/path/to/oiio/install
export ALEMBIC_DIR=/path/to/alembic/config
cmake .. -DAliceVision_DIR=$AV_DIR -DAlembic_DIR=$ALEMBIC_DIR -DCMAKE_PREFIX_PATH=$QT_DIR -DOPENIMAGEIO_LIBRARY_DIR_HINTS:PATH=$OPENIMAGEIO_DIR/lib/ -DOPENIMAGEIO_INCLUDE_DIR:PATH=$OPENIMAGEIO_DIR/include/ -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> -DCMAKE_BUILD_TYPE=Release
make install
```

#### CMake Options

* `BUILD_IMAGEIO_QTHANDLER` (default `ON`)
  Build qtAliceVision_imageIO_qtHandler plugin.

* `BUILD_IMAGEIO` (default `ON`)
  Build qtAliceVision_imageIO plugin.

* `BUILD_SFM` (default `ON`)
  Build qtAliceVision plugin.

* `BUILD_ALEMBIC` (default `ON`)
  Build alembicEntityQml plugin.

## Usage

Once built, setup those environment variables before launching your application:

#### Windows

```bash
set QML2_IMPORT_PATH=<INSTALL_PATH>/qml;%QML2_IMPORT_PATH%
set QT_PLUGIN_PATH=<INSTALL_PATH>;%QT_PLUGIN_PATH%
```

#### Linux

```bash
export QML2_IMPORT_PATH=<INSTALL_PATH>/qml:$QML2_IMPORT_PATH
export QT_PLUGIN_PATH=<INSTALL_PATH>:$QT_PLUGIN_PATH
```
