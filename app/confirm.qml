import QtQuick
import QtQuick.Window

Window {
    id: confirmWindow
    property string actionLabel: ""
    property bool handled: false
    width: Screen.desktopAvailableWidth * 0.23
    height: Screen.desktopAvailableHeight * 0.19
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.Dialog | Qt.WindowStaysOnTopHint
    title: "Ulti Jarvis"
    x: (Screen.desktopAvailableWidth - width) / 2
    y: (Screen.desktopAvailableHeight - height) / 2

    FontLoader {
        id: jakartaFont
        source: "qrc:/fonts/PlusJakartaSans-Bold.ttf"
    }

    function dismiss(confirmed) {
        handled = true
        if (confirmed)
            backend.confirmPower()
        else
            backend.cancelPower()
        confirmWindow.close()
    }

    component GlassButton: Item {
        id: btn
        property string label: ""
        property color accent: "#ffffff"
        property real accentAlpha: 0.0
        property string fontFamily: ""
        signal activated

        Rectangle {
            id: halo
            anchors.centerIn: parent
            width: parent.width + parent.height * 0.5
            height: parent.height + parent.height * 0.5
            radius: height / 2
            color: btn.accent
            opacity: btn.accentAlpha * (area.containsMouse ? 0.22 : 0.12)
            Behavior on opacity { NumberAnimation { duration: 180 } }
        }
        Rectangle {
            id: face
            anchors.fill: parent
            radius: height / 2
            color: Qt.rgba(btn.accent.r, btn.accent.g, btn.accent.b,
                           btn.accentAlpha > 0
                           ? (area.pressed ? 0.92 : area.containsMouse ? 0.82 : 0.72)
                           : (area.pressed ? 0.16 : area.containsMouse ? 0.12 : 0.07))
            border.color: btn.accentAlpha > 0 ? "#59ffffff" : "#2effffff"
            border.width: 1
            Behavior on color { ColorAnimation { duration: 160 } }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#26ffffff" }
                    GradientStop { position: 0.5; color: "#08ffffff" }
                    GradientStop { position: 1.0; color: "#00ffffff" }
                }
            }
            Text {
                anchors.centerIn: parent
                text: btn.label
                color: "#ffffff"
                font.family: btn.fontFamily
                font.pixelSize: btn.height * 0.36
                font.letterSpacing: 0.4
            }
        }
        scale: area.pressed ? 0.97 : 1.0
        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            onClicked: btn.activated()
        }
    }

    Item {
        id: root
        anchors.fill: parent
        focus: true
        Keys.onEscapePressed: confirmWindow.dismiss(false)

        opacity: 0
        scale: 0.94
        Component.onCompleted: {
            root.opacity = 1
            root.scale = 1
        }
        Behavior on opacity { NumberAnimation { duration: 220 } }
        Behavior on scale {
            NumberAnimation { duration: 320; easing.type: Easing.OutBack; easing.overshoot: 1.1 }
        }

        Rectangle {
            id: panel
            anchors.fill: parent
            anchors.margins: confirmWindow.height * 0.05
            radius: confirmWindow.height * 0.12
            border.color: "#2effffff"
            border.width: 1
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#801c1c22" }
                GradientStop { position: 1.0; color: "#85121215" }
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#1cffffff" }
                    GradientStop { position: 0.45; color: "#05ffffff" }
                    GradientStop { position: 1.0; color: "#00ffffff" }
                }
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 1
                width: parent.width * 0.55
                height: 1
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#00ffffff" }
                    GradientStop { position: 0.5; color: "#4dffffff" }
                    GradientStop { position: 1.0; color: "#00ffffff" }
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: panel.height * 0.15

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: panel.width * 0.84
                    text: "Confirm if this is your desired action"
                    color: "#f2ffffff"
                    font.family: jakartaFont.name
                    font.pixelSize: panel.height * 0.105
                    font.letterSpacing: 0.3
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: panel.width * 0.045

                    GlassButton {
                        width: panel.width * 0.34
                        height: panel.height * 0.23
                        label: confirmWindow.actionLabel
                        fontFamily: jakartaFont.name
                        accent: "#e05555"
                        accentAlpha: 1.0
                        onActivated: confirmWindow.dismiss(true)
                    }
                    GlassButton {
                        width: panel.width * 0.34
                        height: panel.height * 0.23
                        label: "Cancel"
                        fontFamily: jakartaFont.name
                        accent: "#ffffff"
                        onActivated: confirmWindow.dismiss(false)
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            z: -1
            property point press
            onPressed: function (mouse) { press = Qt.point(mouse.x, mouse.y) }
            onPositionChanged: function (mouse) {
                confirmWindow.x += mouse.x - press.x
                confirmWindow.y += mouse.y - press.y
            }
        }
    }

    onClosing: if (!handled) backend.cancelPower()
}
