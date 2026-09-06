import QtQuick
import QtQuick.Window
import QtQuick.Controls.Basic

Window {
    id: window
    visible: true
    width: Screen.desktopAvailableWidth * 0.22
    height: Screen.desktopAvailableHeight * 0.42
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    title: "Ulti Jarvis"
    property string panel: ""
    property bool advInner: false
    property bool uninInner: false
    Component.onCompleted: if (!settings.customkey) panel = "welcome"
    FontLoader {
        id: interFont
        source: "qrc:/fonts/Inter_18pt-Black.ttf"
    }
    FontLoader {
        id: jakartaFont
        source: "qrc:/fonts/PlusJakartaSans-Bold.ttf"
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: '#ce1d1b1b' }
            GradientStop { position: 0.5; color: '#e52c2c2e' }
            GradientStop { position: 1.0; color: '#141416' }
        }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.08
        height: parent.height * 0.11
        text: "Ulti Jarvis"
        color: "#ffffff"
        font.family: interFont.name
        font.pixelSize: parent.height * 0.075
        verticalAlignment: Text.AlignVCenter
    }
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.2
        spacing: parent.height * 0.02
        Button {
            width: window.width * 0.7
            height: window.height * 0.13
            onClicked: panel = "mic"
            background: Rectangle {
                radius: window.width * 0.03
                color: parent.pressed ? "#2effffff" : "#12ffffff"
                border.color: "#26ffffff"
                border.width: 1
                Behavior on color { ColorAnimation { duration: 100 } }
            }
            contentItem: Text {
                text: qsTr("Microphone")
                color: "#ffffff"
                font.family: jakartaFont.name
                font.pixelSize: window.height * 0.035
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        Button {
            width: window.width * 0.7
            height: window.height * 0.13
            onClicked: panel = "lang"
            background: Rectangle {
                radius: window.width * 0.03
                color: parent.pressed ? "#2effffff" : "#12ffffff"
                border.color: "#26ffffff"
                border.width: 1
                Behavior on color { ColorAnimation { duration: 100 } }
            }
            contentItem: Text {
                text: qsTr("Language")
                color: "#ffffff"
                font.family: jakartaFont.name
                font.pixelSize: window.height * 0.035
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        Button {
            width: window.width * 0.7
            height: window.height * 0.13
            onClicked: {
                advInner = false
                panel = "adv"
            }
            background: Rectangle {
                radius: window.width * 0.03
                color: parent.pressed ? "#2effffff" : "#12ffffff"
                border.color: "#26ffffff"
                border.width: 1
                Behavior on color { ColorAnimation { duration: 100 } }
            }
            contentItem: Text {
                text: qsTr("Advanced")
                color: "#ffffff"
                font.family: jakartaFont.name
                font.pixelSize: window.height * 0.035
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        Button {
            width: window.width * 0.7
            height: window.height * 0.13
            onClicked: panel = "daemon"
            background: Rectangle {
                radius: window.width * 0.03
                color: parent.pressed ? "#2effffff" : "#12ffffff"
                border.color: "#26ffffff"
                border.width: 1
                Behavior on color { ColorAnimation { duration: 100 } }
            }
            contentItem: Text {
                text: qsTr("Manage Daemon")
                color: "#ffffff"
                font.family: jakartaFont.name
                font.pixelSize: window.height * 0.035
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        Button {
            width: window.width * 0.7
            height: window.height * 0.13
            onClicked: {
                uninInner = false
                panel = "uninstall"
            }
            background: Rectangle {
                radius: window.width * 0.03
                color: parent.pressed ? "#b5620808" : '#cec70000'
                border.color: "#26ffffff"
                border.width: 1
                Behavior on color { ColorAnimation { duration: 100 } }
            }
            contentItem: Text {
                text: qsTr("Uninstall")
                color: "#ffffff"
                font.family: jakartaFont.name
                font.pixelSize: window.height * 0.035
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    Rectangle {
        anchors.fill: parent
        color: "#cc000000"
        visible: panel !== ""
        MouseArea {
            anchors.fill: parent
            onClicked: panel = ""
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.85
            height: Math.min(panelColumn.implicitHeight + parent.height * 0.08, parent.height * 0.85)
            radius: parent.width * 0.03
            color: "#2c2c2e"
            border.color: "#26ffffff"
            border.width: 1
            MouseArea {
                anchors.fill: parent
            }
            Flickable {
                anchors.fill: parent
                anchors.margins: window.width * 0.04
                contentHeight: panelColumn.implicitHeight
                clip: true
                Column {
                    id: panelColumn
                    width: parent.width
                    spacing: window.height * 0.02
                    Text {
                        text: panel === "welcome" ? qsTr("Get Started") : panel === "mic" ? qsTr("Select Microphone") : panel === "lang" ? qsTr("Select Language") : panel === "uninstall" ? qsTr("Uninstall") : panel === "daemon" ? qsTr("Manage Daemon") : qsTr("Advanced")
                        color: "#99ffffff"
                        font.family: jakartaFont.name
                        font.pixelSize: window.height * 0.03
                    }
                    Text {
                        visible: panel === "welcome"
                        width: panelColumn.width
                        text: qsTr("Ulti Jarvis needs a free Groq API key to understand your voice commands. It only takes a minute to get one.")
                        color: "#ffffff"
                        font.family: jakartaFont.name
                        font.pixelSize: window.height * 0.026
                        wrapMode: Text.Wrap
                    }
                    Button {
                        visible: panel === "welcome"
                        width: panelColumn.width
                        height: window.height * 0.09
                        onClicked: Qt.openUrlExternally("https://console.groq.com/keys")
                        background: Rectangle {
                            radius: window.width * 0.02
                            color: parent.pressed ? "#3357d98c" : "#1a57d98c"
                            border.color: "#6657d98c"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: qsTr("Get a Free API Key")
                            color: "#ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.026
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Button {
                        visible: panel === "welcome"
                        width: panelColumn.width
                        height: window.height * 0.09
                        onClicked: {
                            advInner = true
                            panel = "adv"
                        }
                        background: Rectangle {
                            radius: window.width * 0.02
                            color: parent.pressed ? "#2effffff" : "#12ffffff"
                            border.color: "#26ffffff"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: qsTr("I Already Have a Key")
                            color: "#ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.026
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Button {
                        visible: panel === "welcome"
                        width: panelColumn.width
                        height: window.height * 0.08
                        onClicked: panel = ""
                        background: Rectangle {
                            radius: window.width * 0.02
                            color: "transparent"
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: qsTr("Not Now")
                            color: "#77ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.024
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Repeater {
                        model: panel === "daemon" ? [["end", qsTr("End Current Process")], ["restart", qsTr("Restart Daemon")]] : []
                        delegate: Button {
                            width: panelColumn.width
                            height: window.height * 0.09
                            onClicked: {
                                if (modelData[0] === "end")
                                    settings.end_daemon()
                                else
                                    settings.restart_daemon()
                                panel = ""
                            }
                            background: Rectangle {
                                radius: window.width * 0.02
                                color: parent.pressed ? "#2effffff" : "#12ffffff"
                                border.color: "#26ffffff"
                                border.width: 1
                                Behavior on color { ColorAnimation { duration: 100 } }
                            }
                            contentItem: Text {
                                text: modelData[1]
                                color: "#ffffff"
                                font.family: jakartaFont.name
                                font.pixelSize: window.height * 0.026
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    Repeater {
                        model: panel === "mic" ? settings.devicenames : []
                        delegate: Button {
                            width: panelColumn.width
                            height: window.height * 0.09
                            onClicked: {
                                settings.select_device(index)
                                panel = ""
                            }
                            background: Rectangle {
                                radius: window.width * 0.02
                                color: index === settings.deviceindex ? "#3357d98c" : parent.pressed ? "#2effffff" : "#12ffffff"
                                border.color: index === settings.deviceindex ? "#6657d98c" : "#26ffffff"
                                border.width: 1
                                Behavior on color { ColorAnimation { duration: 100 } }
                            }
                            contentItem: Text {
                                width: panelColumn.width * 0.9
                                text: modelData
                                color: "#ffffff"
                                font.family: jakartaFont.name
                                font.pixelSize: window.height * 0.026
                                wrapMode: Text.Wrap
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    Repeater {
                        model: panel === "lang" ? [["en", "English"]] : []
                        delegate: Button {
                            width: panelColumn.width
                            height: window.height * 0.09
                            onClicked: {
                                settings.set_language(modelData[0])
                                panel = ""
                            }
                            background: Rectangle {
                                radius: window.width * 0.02
                                color: modelData[0] === settings.language ? "#3357d98c" : parent.pressed ? "#2effffff" : "#12ffffff"
                                border.color: modelData[0] === settings.language ? "#6657d98c" : "#26ffffff"
                                border.width: 1
                                Behavior on color { ColorAnimation { duration: 100 } }
                            }
                            contentItem: Text {
                                text: modelData[1]
                                color: "#ffffff"
                                font.family: jakartaFont.name
                                font.pixelSize: window.height * 0.026
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    Text {
                        visible: panel === "adv"
                        width: panelColumn.width
                        text: qsTr("This section is for advanced users.")
                        color: "#ffffff"
                        font.family: jakartaFont.name
                        font.pixelSize: window.height * 0.026
                        wrapMode: Text.Wrap
                    }
                    Button {
                        visible: panel === "adv" && !advInner
                        width: panelColumn.width
                        height: window.height * 0.09
                        onClicked: advInner = true
                        background: Rectangle {
                            radius: window.width * 0.02
                            color: parent.pressed ? "#2effffff" : "#12ffffff"
                            border.color: "#26ffffff"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: qsTr("API key management")
                            color: "#ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.026
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Text {
                        visible: panel === "adv" && advInner
                        width: panelColumn.width
                        text: qsTr("Only Groq API keys are supported.")
                        color: "#e0b055"
                        font.family: jakartaFont.name
                        font.pixelSize: window.height * 0.024
                        wrapMode: Text.Wrap
                    }
                    Text {
                        visible: panel === "adv" && advInner
                        width: panelColumn.width
                        text: settings.customkey ? qsTr("Using custom key") : qsTr("Using default key")
                        color: "#99ffffff"
                        font.family: jakartaFont.name
                        font.pixelSize: window.height * 0.022
                        wrapMode: Text.Wrap
                    }
                    Rectangle {
                        visible: panel === "adv" && advInner
                        width: panelColumn.width
                        height: window.height * 0.08
                        radius: window.width * 0.02
                        color: "#12ffffff"
                        border.color: "#26ffffff"
                        border.width: 1
                        TextInput {
                            id: keyInput
                            anchors.fill: parent
                            anchors.margins: window.width * 0.02
                            color: "#ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.024
                            clip: true
                            verticalAlignment: TextInput.AlignVCenter
                            echoMode: TextInput.Password
                            Text {
                                anchors.fill: parent
                                verticalAlignment: Text.AlignVCenter
                                text: qsTr("Your Groq API key")
                                color: "#66ffffff"
                                font.family: jakartaFont.name
                                font.pixelSize: window.height * 0.024
                                visible: keyInput.text === "" && !keyInput.activeFocus
                            }
                        }
                    }
                    Button {
                        visible: panel === "adv" && advInner
                        width: panelColumn.width
                        height: window.height * 0.08
                        onClicked: {
                            settings.set_apikey(keyInput.text)
                            keyInput.text = ""
                            panel = ""
                        }
                        background: Rectangle {
                            radius: window.width * 0.02
                            color: parent.pressed ? "#3357d98c" : "#1a57d98c"
                            border.color: "#6657d98c"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: qsTr("Save")
                            color: "#ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.026
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Button {
                        visible: panel === "adv" && advInner
                        width: panelColumn.width
                        height: window.height * 0.08
                        onClicked: {
                            settings.reset_apikey()
                            keyInput.text = ""
                            panel = ""
                        }
                        background: Rectangle {
                            radius: window.width * 0.02
                            color: parent.pressed ? "#33e05555" : "#1ae05555"
                            border.color: "#66e05555"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: qsTr("Reset to default")
                            color: "#ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.026
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Text {
                        visible: panel === "uninstall" && !uninInner
                        width: panelColumn.width
                        text: qsTr("This will completely remove Ulti Jarvis and all of its data from this device.")
                        color: "#ffffff"
                        font.family: jakartaFont.name
                        font.pixelSize: window.height * 0.026
                        wrapMode: Text.Wrap
                    }
                    Button {
                        visible: panel === "uninstall" && !uninInner
                        width: panelColumn.width
                        height: window.height * 0.09
                        onClicked: uninInner = true
                        background: Rectangle {
                            radius: window.width * 0.02
                            color: parent.pressed ? "#33e05555" : "#1ae05555"
                            border.color: "#66e05555"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: qsTr("Uninstall Ulti Jarvis")
                            color: "#ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.026
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Text {
                        visible: panel === "uninstall" && uninInner
                        width: panelColumn.width
                        text: qsTr("This action cannot be undone. Are you sure?")
                        color: "#e0b055"
                        font.family: jakartaFont.name
                        font.pixelSize: window.height * 0.024
                        wrapMode: Text.Wrap
                    }
                    Button {
                        visible: panel === "uninstall" && uninInner
                        width: panelColumn.width
                        height: window.height * 0.08
                        onClicked: settings.uninstall()
                        background: Rectangle {
                            radius: window.width * 0.02
                            color: parent.pressed ? "#33e05555" : "#1ae05555"
                            border.color: "#66e05555"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: qsTr("Uninstall")
                            color: "#ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.026
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Button {
                        width: panelColumn.width
                        height: window.height * 0.08
                        onClicked: panel = ""
                        background: Rectangle {
                            radius: window.width * 0.02
                            color: parent.pressed ? "#2effffff" : "#12ffffff"
                            border.color: "#26ffffff"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: qsTr("Close")
                            color: "#ffffff"
                            font.family: jakartaFont.name
                            font.pixelSize: window.height * 0.026
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }
    }
}
