import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var theme
    required property string labelText
    required property real red
    required property real green
    required property real blue

    signal colorEdited(real red, real green, real blue)

    property var swatchColors: [
        "#FFFFFF", "#D9D9D9", "#A6A6A6", "#737373", "#404040", "#000000",
        "#FF4D4F", "#FA8C16", "#FADB14", "#52C41A", "#13C2C2", "#1677FF",
        "#2F54EB", "#722ED1", "#EB2F96", "#F5222D", "#FA541C", "#FAAD14"
    ]

    radius: 10
    color: root.theme.window
    border.color: root.theme.border
    implicitHeight: contentColumn.implicitHeight + 20

    function clampByte(value) {
        return Math.max(0, Math.min(255, Math.round(value)))
    }

    function channelToByte(value) {
        return clampByte((value || 0) * 255)
    }

    function byteToChannel(value) {
        return clampByte(value) / 255.0
    }

    function toHexByte(value) {
        return clampByte(value).toString(16).toUpperCase().padStart(2, "0")
    }

    function currentHex() {
        return "#"
                + toHexByte(channelToByte(root.red))
                + toHexByte(channelToByte(root.green))
                + toHexByte(channelToByte(root.blue))
    }

    function applyRgbBytes(redByte, greenByte, blueByte) {
        root.colorEdited(byteToChannel(redByte),
                         byteToChannel(greenByte),
                         byteToChannel(blueByte))
    }

    function applyHexText(value) {
        const normalized = String(value || "").trim()
        const match = /^#?([0-9A-Fa-f]{6})$/.exec(normalized)
        if (!match) {
            return false
        }

        const parsed = match[1]
        const redByte = parseInt(parsed.slice(0, 2), 16)
        const greenByte = parseInt(parsed.slice(2, 4), 16)
        const blueByte = parseInt(parsed.slice(4, 6), 16)

        applyRgbBytes(redByte, greenByte, blueByte)
        return true
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: root.labelText
            color: root.theme.textSecondary
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 52
                Layout.preferredHeight: 36
                radius: 8
                color: Qt.rgba(root.red, root.green, root.blue, 1.0)
                border.color: root.theme.border
            }

            TextField {
                id: hexInput
                Layout.fillWidth: true
                text: root.currentHex()
                placeholderText: "#RRGGBB"
                selectByMouse: true

                onAccepted: {
                    if (!root.applyHexText(text)) {
                        text = root.currentHex()
                    } else {
                        text = root.currentHex()
                    }
                }

                onEditingFinished: {
                    if (!root.applyHexText(text)) {
                        text = root.currentHex()
                    } else {
                        text = root.currentHex()
                    }
                }
            }
        }

        RowLayout {
            id: rgbRow
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            Label { text: "R"; color: root.theme.textSecondary }
            SpinBox {
                id: redSpin
                from: 0
                to: 255
                editable: true
                value: root.channelToByte(root.red)
                onValueModified: root.applyRgbBytes(value, greenSpin.value, blueSpin.value)
            }

            Label { text: "G"; color: root.theme.textSecondary }
            SpinBox {
                id: greenSpin
                from: 0
                to: 255
                editable: true
                value: root.channelToByte(root.green)
                onValueModified: root.applyRgbBytes(redSpin.value, value, blueSpin.value)
            }

            Label { text: "B"; color: root.theme.textSecondary }
            SpinBox {
                id: blueSpin
                from: 0
                to: 255
                editable: true
                value: root.channelToByte(root.blue)
                onValueModified: root.applyRgbBytes(redSpin.value, greenSpin.value, value)
            }
        }

        Grid {
            id: swatchGrid
            Layout.alignment: Qt.AlignHCenter
            columns: contentColumn.width >= 560 ? 18 : (contentColumn.width >= 420 ? 9 : 6)
            spacing: 6

            Repeater {
                model: root.swatchColors

                delegate: Rectangle {
                    required property var modelData

                    width: 22
                    height: 22
                    radius: 11
                    color: modelData
                    border.color: root.theme.border

                    TapHandler {
                        onTapped: root.applyHexText(parent.modelData)
                    }
                }
            }
        }
    }
}
