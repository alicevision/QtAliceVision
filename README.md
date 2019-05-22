# QtAliceVision - AliceVision QML plugin

QtAliceVision is a C++ QML plugin providing classes to load and visualize AliceVision data.

Currently available Viewers:
  - [X] Features 
  - [] Matches


## License

The project is released under MPLv2, see [**COPYING.md**](COPYING.md).


## Get the project

Get the source code:
```bash
git clone --recursive git://github.com/alicevision/QtAliceVision
cd QtAliceVision
```
See [**INSTALL.md**](INSTALL.md) to build and install the project.


## Usage

Once built and with the plugin installation folder in `QML2_IMPORT_PATH`:

```js
import AliceVision 1.0

FeaturesViewer {
    describerType: "sift"
    color: "red"
    displayMode: FeaturesViewer.Points
    folder: "/path/to/features/folder"
    viewId: 101245654
}
```
