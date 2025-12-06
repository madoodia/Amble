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

        Item {
            SplitView.fillWidth: true
            SplitView.preferredWidth: parent.width / 2

            CircuitViewport {
                id: circuitViewport
                anchors.fill: parent

                backgroundColor: '#232327'
                gridColor: '#898989'
                gridSize: 20

                Component.onCompleted: {
                    console.log("CircuitViewport QML completed with size:", width, "x", height);
                }
            }

            // MouseArea to handle all mouse interactions
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons

                property bool dragging: false
                property point lastMousePos: Qt.point(0, 0)

                onPressed: function (mouse) {
                    lastMousePos = Qt.point(mouse.x, mouse.y);

                    if (mouse.button === Qt.LeftButton) {
                        if (mouse.modifiers & Qt.ControlModifier) {
                            // Ctrl+click for wire connections
                            var componentId = circuitViewport.getComponentAtPosition(mouse.x, mouse.y);
                            if (componentId >= 0) {
                                circuitViewport.handleWireConnection(componentId);
                            }
                        } else {
                            circuitViewport.selectComponent(mouse.x, mouse.y);
                            dragging = true;
                        }
                    } else if (mouse.button === Qt.MiddleButton) {
                        // Start panning
                        dragging = false; // Make sure we're not in component drag mode
                    }
                }

                onReleased: function (mouse) {
                    if (mouse.button === Qt.RightButton) {
                        console.log("Right click detected at:", mouse.x, mouse.y);
                        contextMenu.x = mouse.x;
                        contextMenu.y = mouse.y;
                        contextMenu.open();
                    } else if (mouse.button === Qt.LeftButton && dragging) {
                        // Snap to grid when drag ends
                        circuitViewport.snapSelectedToGrid();
                    }
                    dragging = false;
                }

                onPositionChanged: function (mouse) {
                    var deltaX = mouse.x - lastMousePos.x;
                    var deltaY = mouse.y - lastMousePos.y;

                    if (dragging && mouse.buttons & Qt.LeftButton) {
                        circuitViewport.moveSelectedComponents(deltaX, deltaY);
                    } else if (mouse.buttons & Qt.MiddleButton) {
                        // Pan the viewport
                        var currentPan = circuitViewport.panOffset;
                        circuitViewport.panOffset = Qt.point(currentPan.x + deltaX, currentPan.y + deltaY);
                    }
                    lastMousePos = Qt.point(mouse.x, mouse.y);
                }

                onWheel: function (wheel) {
                    var zoomFactor = wheel.angleDelta.y > 0 ? 1.1 : 0.9;
                    circuitViewport.zoom = circuitViewport.zoom * zoomFactor;
                }
            }

            // Context Menu
            Menu {
                id: contextMenu

                MenuItem {
                    text: "Add Resistor"
                    onTriggered: {
                        circuitViewport.addComponent("Resistor", contextMenu.x, contextMenu.y);
                        console.log("Added Resistor at", contextMenu.x, contextMenu.y);
                    }
                }

                MenuItem {
                    text: "Add Capacitor"
                    onTriggered: {
                        circuitViewport.addComponent("Capacitor", contextMenu.x, contextMenu.y);
                        console.log("Added Capacitor at", contextMenu.x, contextMenu.y);
                    }
                }

                MenuItem {
                    text: "Add Inductor"
                    onTriggered: {
                        circuitViewport.addComponent("Inductor", contextMenu.x, contextMenu.y);
                        console.log("Added Inductor at", contextMenu.x, contextMenu.y);
                    }
                }

                MenuItem {
                    text: "Add Voltage Source"
                    onTriggered: {
                        circuitViewport.addComponent("Voltage Source", contextMenu.x, contextMenu.y);
                        console.log("Added Voltage Source at", contextMenu.x, contextMenu.y);
                    }
                }

                MenuSeparator {}

                MenuItem {
                    text: "Clear All Components"
                    onTriggered: {
                        circuitViewport.clearComponents();
                        console.log("Cleared all components");
                    }
                }
            }
        }

        Rectangle {
            id: propertiesPanel

            SplitView.fillWidth: true
            SplitView.preferredWidth: parent.width / 3
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

                Text {
                    text: "Grid Size: " + Math.round(gridSizeSlider.value)
                    color: "white"
                }

                Slider {
                    id: gridSizeSlider
                    from: 5
                    to: 100
                    value: circuitViewport.gridSize
                    onValueChanged: {
                        circuitViewport.gridSize = value;
                    }
                }

                Text {
                    text: "Components"
                    color: "white"
                    font.bold: true
                    topPadding: 20
                }

                Button {
                    text: "Clear All"
                    onClicked: {
                        circuitViewport.clearComponents();
                    }
                }

                Text {
                    text: "Zoom: " + Math.round(circuitViewport.zoom * 100) + "%"
                    color: "white"
                    font.bold: true
                    topPadding: 20
                }

                Slider {
                    id: zoomSlider
                    from: 0.1
                    to: 5.0
                    value: circuitViewport.zoom
                    onValueChanged: {
                        circuitViewport.zoom = value;
                    }
                }

                Button {
                    text: "Reset View"
                    onClicked: {
                        circuitViewport.zoom = 1.0;
                        circuitViewport.panOffset = Qt.point(0, 0);
                    }
                }

                Text {
                    text: "Controls:"
                    color: "#cccccc"
                    font.bold: true
                    topPadding: 20
                }

                Text {
                    text: "• Right-click: Add components\n• Left-click: Select/drag components\n• Middle-click: Pan view\n• Mouse wheel: Zoom\n• Ctrl+click: Connect with wires"
                    color: "#cccccc"
                    wrapMode: Text.WordWrap
                    width: parent.width - 20
                    font.pointSize: 9
                }
            }
        }
    }
}
