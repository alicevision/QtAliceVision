# Development
This guide will help you build and install QtAliceVision plugin.

## Requirements
QtAliceVision requires:
* [Qt6](https://www.qt.io/) (== 6.8.3, make sure to use the **same version** as the target application)
* [AliceVision](https://github.com/aliceVision/AliceVision)
* [OpenImageIO](https://github.com/AcademySoftwareFoundation/OpenImageIO) (>= 2.1, recommended >= 2.4.13) - with OpenEXR support for depth maps visualization
* [Alembic](https://github.com/alembic/alembic) (>= 1.8.5)
* [CMake](https://cmake.org/) (>= 3.11)
* On Windows platform: Microsoft Visual Studio (>= 2022)

> [!NOTE]
For Windows, we recommend using [VCPKG](https://github.com/Microsoft/vcpkg) to get Alembic. Qt can either be installed with VCPKG or the official installer.


## Build instructions

In the following steps, replace <INSTALL_PATH> with the installation path of your choice.


### Windows

> [!NOTE]
> We will use "NMake Makefiles" generators here to have one-line build/installation, but Visual Studio solutions can be generated if need be.

From a developer command-line, using Qt 6.8.3 (installed through the official installer) and VCPKG for OIIO:
```
set QT_DIR=/path/to/Qt/6.8.3/msvc2022_64
set AV_DIR=/path/to/aliceVision/cmake/folder
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DAliceVision_DIR=%AV_DIR% -DCMAKE_PREFIX_PATH=%QT_DIR% -DVCPKG_TARGET_TRIPLET=x64-windows-release -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> 
nmake install
```

### Linux

```bash
export QT_DIR=/path/to/Qt/6.8.3/gcc_64
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
