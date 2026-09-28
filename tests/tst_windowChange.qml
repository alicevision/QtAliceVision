import QtCore
import QtQuick
import QtQuick.Window
import QtTest
import AliceVision 1.0 as AliceVision

/**
 * A viewer moved into another window must render its image again: its scene graph node is
 * re-created in the new window, and used to stay blank until the image changed.
 */
Item {
    id: root
    width: 64
    height: 48

    readonly property string tempDir: {
        const url = StandardPaths.writableLocation(StandardPaths.TempLocation).toString()
        return decodeURIComponent(url.replace(/^file:\/\//, "").replace(/^\/([A-Za-z]:)/, "$1"))
    }
    readonly property string albedoPath: tempDir + "/qtAliceVision_windowChange_albedo.png"
    readonly property string normalsPath: tempDir + "/qtAliceVision_windowChange_normals.png"

    Rectangle { id: albedo; width: 64; height: 48; color: "#ff0000" }
    // Normals (0, 1, 0): lit by a light pitched by 90 degrees (a PNG cannot hold a negative z)
    Rectangle { id: normals; width: 64; height: 48; color: "#00ff00" }

    Window { id: windowA; width: 200; height: 150; visible: true }
    Window { id: windowB; x: 250; width: 200; height: 150; visible: true }

    Component {
        id: floatImageViewer
        AliceVision.FloatImageViewer {
            width: sourceSize.width  // as in Meshroom: the surface is in image coordinates
            height: sourceSize.height
            source: Qt.url("file://" + root.albedoPath)
            surface.subdivisions: 1
            function loaded() { return status === AliceVision.FloatImageViewer.EStatus.NONE && sourceSize.width > 0 }
        }
    }

    Component {
        id: phongImageViewer
        AliceVision.PhongImageViewer {
            width: sourceSize.width
            height: sourceSize.height
            sourcePath: Qt.url("file://" + root.albedoPath)
            normalPath: Qt.url("file://" + root.normalsPath)
            textureOpacity: 1.0
            kd: 1.0
            ka: 0.0
            ks: 0.0
            lightPitch: 90.0
            function loaded() { return status === AliceVision.PhongImageViewer.EStatus.NONE && sourceSize.width > 0 }
        }
    }

    TestCase {
        name: "WindowChange"
        when: windowShown

        function initTestCase() {
            let saved = 0
            albedo.grabToImage(function(result) { result.saveToFile(root.albedoPath); saved++ })
            normals.grabToImage(function(result) { result.saveToFile(root.normalsPath); saved++ })
            tryVerify(function() { return saved === 2 }, 5000, "test images saved")
        }

        function isRed(viewer) {
            const image = grabImage(viewer)
            const x = Math.floor(image.width / 2)
            const y = Math.floor(image.height / 2)
            return image.red(x, y) > 200 && image.green(x, y) < 60 && image.blue(x, y) < 60
        }

        function test_windowChange_data() {
            return [
                { tag: "FloatImageViewer", component: floatImageViewer },
                { tag: "PhongImageViewer", component: phongImageViewer },
            ]
        }

        function test_windowChange(data) {
            const viewer = createTemporaryObject(data.component, windowA.contentItem)
            verify(viewer)
            tryVerify(viewer.loaded, 10000, "image loaded")
            tryVerify(function() { return isRed(viewer) }, 5000, "image rendered in the first window")

            viewer.parent = windowB.contentItem
            tryVerify(function() { return isRed(viewer) }, 5000, "image rendered after moving to another window")

            viewer.parent = windowA.contentItem
            tryVerify(function() { return isRed(viewer) }, 5000, "image rendered after moving back")
        }
    }
}
