# QtAliceVision - AliceVision QML plugin

QtAliceVision is a C++ QML plugin providing classes to load and visualize AliceVision data.

Currently available Viewers:
  - [X] Features 
    - Position, scale and orientation 
    - Status: extracted, matched or reconstructed 
  - [ ] Matches 
  - [X] Statistics 
    - Per view statistics with reprojection error, observations histograms 
    - Global SfM statistics with number of landmarks/matches, reprojection errors, observations per view 
  - [X] Images 
    - Visualize images with floating point precision 
    - Dynamically adjust gain and gamma


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

 - Create an `MSfMData` object to get access to the SfM information:

```js
import AliceVision 1.0

MSfMData {
  id: sfmData
  sfmDataPath: "/path/to/SfM/sfmData.abc”
}
```

 - Create an `MTracks` to load all matches and get access to tracks information:

```js
import AliceVision 1.0

MTracks {
  id: tracks
  matchingFolder: "/path/to/FeatureMatching/UID/”
}
```

 - Create a `MFeatures` object to get access to all features data:

```js
MFeatures {
  viewId: 101245654
  describerTypes: ["sift", "akaze"]
  featureFolder: "/path/to/features/folder"
  mTracks: tracks
  mSfmData: sfmData
}
```

 - Create a `FeaturesViewer` to visualize features position, scale, orientation and optionally information about the feature status regarding tracks and SfmData.

```js
FeaturesViewer {
    colorOffset: 0
    describerType: "sift"
    color: “blue”
    landmarkColor: “red”
    displayMode: FeaturesViewer.Points
    mdescFeatures: mfeatures.allFeatures(describerType)
}
```

 - Create an `MSfMDataStats` to display global statistics about your SfMData:

```js

MSfMDataStats {
  msfmData: msfmData
  mTracks: mTracks
}
```

 - Create an `MViewStats` to display statistics about a specific View of your SfMData:

```js
MViewStats {
  msfmData: msfmData
  viewId: 451244710
}
```

 - Create a `FloatImageViewer` to display an image with floating point precision, allowing to dynamically adjust the gain and gamma:

```js
FloatImageViewer {
  source: "/path/to/image"
  gamma: 1.0f
  offset: 0.0f
  width: 500
  height: 540
  paintedWidth: 500
  paintedHeight: 540
  channelMode: “rgb”
}
```

