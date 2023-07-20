# Development
This guide will help you build and install qmlAlembic plugin.

## Requirements
qmlAlembic requires:
* [Qt5](https://www.qt.io/) (== 5.15.2, make sure to use the **same version** as the target application)
* [Alembic](https://github.com/alembic/alembic) (>= 1.7)
* [CMake](https://cmake.org/) (>= 3.3)
* On Windows platform: Microsoft Visual Studio (>= 2015.3)

> **Note for Windows**:
We recommend using [VCPKG](https://github.com/Microsoft/vcpkg) to get Alembic. Qt can either be installed with VCPKG or the official installer.

## Build instructions

In the following steps, replace <INSTALL_PATH> with the installation path of your choice.


#### Windows
> We will use "NMake Makefiles" generators here to have one-line build/installation,
but Visual Studio solutions can be generated if need be.

From a developer command-line, using Qt 5.15.2 (built with VS2019) and VCPKG for Alembic:
```
set QT_DIR=/path/to/qt/5.15.2/msvc2019_64
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DCMAKE_PREFIX_PATH=%QT_DIR% -DVCPKG_TARGET_TRIPLET=x64-windows -G "NMake Makefiles" -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> -DCMAKE_BUILD_TYPE=Release
nmake install
```

#### Linux

```bash
export QT_DIR=/path/to/qt/5.15.2/gcc_64
export ALEMBIC_DIR=/path/to/alembic/config
cmake .. -DAlembic_DIR=$ALEMBIC_DIR -DCMAKE_PREFIX_PATH=$QT_DIR -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> -DCMAKE_BUILD_TYPE=Release
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
