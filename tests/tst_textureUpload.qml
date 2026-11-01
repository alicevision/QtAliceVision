import QtCore
import QtQuick
import QtQuick.Window
import QtTest
import AliceVision 1.0 as AliceVision

/**
 * Float textures reference the pixels of their image when they are uploaded, without copying them:
 * check the displayed pixels always match the current image.
 */
Item {
    id: root
    width: 64
    height: 48

    readonly property string tempDir: {
        const url = StandardPaths.writableLocation(StandardPaths.TempLocation).toString()
        return decodeURIComponent(url.replace(/^file:\/\//, "").replace(/^\/([A-Za-z]:)/, "$1"))
    }
    readonly property url redUrl: Qt.url("file://" + tempDir + "/qtAliceVision_textureUpload_red.png")
    readonly property url blueUrl: Qt.url("file://" + tempDir + "/qtAliceVision_textureUpload_blue.png")
    readonly property url normalsUrl: Qt.url("file://" + tempDir + "/qtAliceVision_textureUpload_normals.png")

    Rectangle { id: red; width: 64; height: 48; color: "#ff0000" }
    Rectangle { id: blue; width: 64; height: 48; color: "#0000ff" }
    // Normals (0, 1, 0): lit by a light pitched by 90 degrees (a PNG cannot hold a negative z)
    Rectangle { id: normals; width: 64; height: 48; color: "#00ff00" }

    Window { id: window; width: 200; height: 150; visible: true }

    Component {
        id: floatImageViewer
        AliceVision.FloatImageViewer {
            property alias imageUrl: viewer.source
            id: viewer
            width: sourceSize.width
            height: sourceSize.height
            surface.subdivisions: 1
            resizeRatio: 1.0  // load at full resolution: FloatTexture does the downscale
        }
    }

    Component {
        id: phongImageViewer
        AliceVision.PhongImageViewer {
            property alias imageUrl: viewer.sourcePath
            id: viewer
            width: sourceSize.width
            height: sourceSize.height
            normalPath: root.normalsUrl
            textureOpacity: 1.0
            kd: 1.0
            ka: 0.0
            ks: 0.0
            lightPitch: 90.0
        }
    }

    TestCase {
        name: "TextureUpload"
        when: windowShown

        function initTestCase() {
            let saved = 0
            const save = function(item, url) {
                item.grabToImage(function(result) { result.saveToFile(url.toString().replace(/^file:\/\//, "")); saved++ })
            }
            save(red, root.redUrl)
            save(blue, root.blueUrl)
            save(normals, root.normalsUrl)
            tryVerify(function() { return saved === 3 }, 5000, "test images saved")
        }

        // Color of a window pixel: "red", "blue" or "other"
        function colorAt(x, y) {
            const image = grabImage(window.contentItem)
            const r = image.red(x, y), g = image.green(x, y), b = image.blue(x, y)
            if (r > 200 && g < 60 && b < 60)
                return "red"
            if (b > 200 && r < 60 && g < 60)
                return "blue"
            return "other"
        }

        function test_imageChanges_data() {
            return [
                { tag: "FloatImageViewer", component: floatImageViewer },
                { tag: "PhongImageViewer", component: phongImageViewer },
            ]
        }

        function test_imageChanges(data) {
            const viewer = createTemporaryObject(data.component, window.contentItem, { imageUrl: root.redUrl })
            verify(viewer)
            for (let i = 0; i < 10; ++i) {
                const expected = (i % 2) ? "blue" : "red"
                viewer.imageUrl = (i % 2) ? root.blueUrl : root.redUrl
                tryVerify(function() { return colorAt(32, 24) === expected }, 10000, "change " + i + ": " + expected + " image rendered")
            }
        }

        // Larger than the maximum texture size of any GPU: downscaled before being uploaded
        function test_oversizedImage() {
            const viewer = createTemporaryObject(floatImageViewer, window.contentItem, { imageUrl: Qt.resolvedUrl("data/oversized_red.png") })
            verify(viewer)
            tryVerify(function() { return viewer.sourceSize.width === 65537 }, 10000, "image loaded at full resolution")
            tryVerify(function() { return colorAt(100, 4) === "red" }, 5000, "downscaled image rendered")
            // Only the texture is downscaled, the image of the viewer keeps its full resolution
            verify(viewer.pixelValueAt(65000, 4).x > 0.9, "full resolution image still readable")
        }
    }
}
