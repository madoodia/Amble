import QtQuick
import QtQuick.Controls
import Amble

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: "Amble - Circuit Design Simulator"

    // Rectangle {
    //     anchors.fill: parent
    //     color: "#1e1e1e"

    //     Text {
    //         anchors.centerIn: parent
    //         text: "Test - Replace with CircuitViewport"
    //         color: "white"
    //     }
    // }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        implicitWidth: 500

        CircuitViewport {
            id: circuitViewport

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: propertiesPanel.left

            visible: true
            backgroundColor: '#232327'
            gridColor: '#898989'
            gridSize: 20

            Component.onCompleted: {
                console.log("CircuitViewport QML completed with size:", width, "x", height);
            }
        }

        Rectangle {
            id: propertiesPanel

            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 300
            color: "#252526"

            Text {
                anchors.centerIn: parent
                text: "Properties Panel"
                color: "white"
            }
            Column {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 10
                spacing: 10

                Slider {
                    id: gridSizeSlider
                    from: 5
                    to: 100
                    value: circuitViewport.gridSize
                    onValueChanged: {
                        circuitViewport.gridSize = value;
                    }
                }
            }
        }
    }
}
