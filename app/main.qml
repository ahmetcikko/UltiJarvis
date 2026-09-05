import QtQuick
import QtQuick.Window

Window {
    id: window
    visible: true
    width: Screen.desktopAvailableWidth * 0.25
    height: Screen.desktopAvailableHeight * 0.3
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.SplashScreen
           | Qt.WindowTransparentForInput | Qt.WindowDoesNotAcceptFocus
    x: (Screen.desktopAvailableWidth - width) / 2
    y: Screen.desktopAvailableHeight - height - Screen.desktopAvailableHeight * 0.011
    title: "Ulti Jarvis"
    property string responseText: ""
    property string responseKind: "chat"
    readonly property real pillShownY: height - pill.height - height * 0.044
    readonly property real pillHiddenY: height + height * 0.03
    function orbColor() {
        if (responseKind === "error" && backend.state === "responding")
            return "#e05555"
        if (backend.state === "thinking") return "#b06cf0"
        if (backend.state === "responding") return "#57d98c"
        return "#4da3ff"
    }
    Rectangle {
        id: bubble
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: pill.top
        anchors.bottomMargin: parent.height * 0.03
        width: Math.min(bubbleText.implicitWidth + parent.width * 0.075, parent.width - parent.width * 0.05)
        height: Math.min(bubbleText.implicitHeight + parent.height * 0.0875, parent.height * 0.65)
        radius: parent.width * 0.0375
        color: "#f0141416"
        border.color: responseKind === "error" ? "#66e05555" : "#26ffffff"
        border.width: 1
        opacity: backend.state === "responding" && responseText !== "" ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
        Text {
            id: bubbleText
            anchors.fill: parent
            anchors.margins: window.width * 0.03
            text: responseText
            color: "#ffffff"
            font.pixelSize: window.height * 0.047
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            clip: true
        }
    }
    Rectangle {
        id: pill
        width: pillRow.implicitWidth + parent.width * 0.09
        height: parent.height * 0.2
        radius: height / 2
        y: pillHiddenY
        anchors.horizontalCenter: parent.horizontalCenter
        color: "#e8141416"
        border.color: "#33ffffff"
        border.width: 1
        Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
        Row {
            id: pillRow
            anchors.centerIn: parent
            spacing: window.width * 0.03
            Item {
                width: window.height * 0.125
                height: window.height * 0.125
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    id: glow
                    anchors.centerIn: parent
                    width: window.height * 0.125
                    height: window.height * 0.125
                    radius: width / 2
                    color: orbColor()
                    opacity: 0.25
                    property real pulse: 1.0
                    scale: backend.state === "listening" ? 1.0 + backend.level * 0.8 : pulse
                    Behavior on scale { NumberAnimation { duration: 120 } }
                    Behavior on color { ColorAnimation { duration: 250 } }
                    SequentialAnimation {
                        id: glowPulse
                        running: backend.state === "thinking"
                        loops: Animation.Infinite
                        onStopped: glow.pulse = 1.0
                        NumberAnimation { target: glow; property: "pulse"; to: 1.45; duration: 550; easing.type: Easing.InOutQuad }
                        NumberAnimation { target: glow; property: "pulse"; to: 1.0; duration: 550; easing.type: Easing.InOutQuad }
                    }
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: window.height * 0.094
                    height: window.height * 0.094
                    radius: width / 2
                    color: orbColor()
                    Behavior on color { ColorAnimation { duration: 250 } }
                    Image {
                        source: "qrc:/images/mic.svg"
                        width: window.height * 0.05
                        height: window.height * 0.05
                        anchors.centerIn: parent
                    }
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: "#ddffffff"
                font.pixelSize: window.height * 0.05
                text: backend.state === "listening" ? "Listening…"
                    : backend.state === "thinking" ? "Thinking…"
                    : backend.state === "responding" ? "Ulti Jarvis"
                    : ""
            }
        }
    }
    SequentialAnimation {
        id: enterAnim
        NumberAnimation {
            target: pill
            property: "y"
            from: pillHiddenY
            to: pillShownY
            duration: 450
            easing.type: Easing.OutBack
            easing.overshoot: 1.2
        }
        ScriptAction { script: backend.startRecording() }
    }
    SequentialAnimation {
        id: exitAnim
        NumberAnimation {
            target: pill
            property: "y"
            to: pillHiddenY
            duration: 380
            easing.type: Easing.InBack
        }
        ScriptAction { script: backend.quitNow() }
    }
    Loader {
        id: confirmLoader
        active: false
        source: "qrc:/app/confirm.qml"
    }
    Connections {
        target: backend
        function onPowerConfirmRequested(label) {
            confirmLoader.active = true
            confirmLoader.item.actionLabel = label
            confirmLoader.item.handled = false
            confirmLoader.item.show()
            confirmLoader.item.raise()
            confirmLoader.item.requestActivate()
        }
        function onResponseReady(text, displayMs, kind) {
            responseText = text
            responseKind = kind
        }
        function onStateChanged() {
            if (backend.state === "closing") {
                responseText = ""
                exitAnim.start()
            }
        }
    }
    Component.onCompleted: enterAnim.start()
}
