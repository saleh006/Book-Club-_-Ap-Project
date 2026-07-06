import QtQuick

Rectangle {
    id: root
    anchors.fill: parent
    color: "#1A1A1A"   // background color
    FontLoader {
        id: rubstampFont
        source: ":/RUBSTAMP.TTF"
    }

    Rectangle {
        id: sticker

        width: 250
        height: 250
        anchors.centerIn: parent
        color: "transparent"

        property var rules: [
            "📖 1st & 2nd Rule: You do not talk about Book Club.",
            "🚨 3rd Rule: If someone yells \"spoiler!\", falls asleep, or taps out, the reading session is over.",
            "👥 4th Rule: Only two readers to a debate.",
            "📚 5th Rule: One book at a time, fellas.",
            "🚫 6th Rule: No bookmarks, no dog-ears.",
            "⏳ 7th Rule: Readings go on as long as they have to.",
            "🆕 8th Rule: If this is your first time at Book Club, you have to read."
        ]

        property int ruleIndex: 0
        property int nextIndex: 1
        property real flipAngle: 0

        // FRONT FACE
        Rectangle {
            id: frontFace
            anchors.fill: parent
            radius: 20
            color: "#7C3E66"

            visible: sticker.flipAngle < 90

            transform: Rotation {
                origin.x: frontFace.width / 2
                origin.y: frontFace.height / 2
                axis.y: 1
                angle: sticker.flipAngle
            }

            Text {
                // Changed to your custom font
                font.family: rubstampFont.name
                anchors.centerIn: parent
                width: parent.width - 20
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap

                text: sticker.rules[sticker.ruleIndex]
                color: "white"
                font.pixelSize: 18
                font.bold: true
                renderType: Text.QtRendering
            }
        }

        // BACK FACE
        Rectangle {
            id: backFace
            anchors.fill: parent
            radius: 20
            color: "#7C3E66"

            visible: sticker.flipAngle >= 90

            transform: Rotation {
                origin.x: backFace.width / 2
                origin.y: backFace.height / 2
                axis.y: 1
                angle: sticker.flipAngle - 180
            }

            Text {
                // Changed to your custom font
                font.family: rubstampFont.name
                anchors.centerIn: parent
                width: parent.width - 20
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap

                text: sticker.rules[sticker.nextIndex]
                color: "white"
                font.pixelSize: 18
                font.bold: true
                renderType: Text.QtRendering
            }
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                if (!flipAnimation.running) {
                    sticker.nextIndex =
                        (sticker.ruleIndex + 1) % sticker.rules.length

                    flipAnimation.start()
                }
            }
        }

        SequentialAnimation {
            id: flipAnimation

            NumberAnimation {
                target: sticker
                property: "flipAngle"
                from: 0
                to: 180
                duration: 500
                easing.type: Easing.InOutQuad
            }

            ScriptAction {
                script: {
                    sticker.ruleIndex = sticker.nextIndex
                    sticker.flipAngle = 0
                }
            }
        }

        Rectangle {
            id: pulseRing
            anchors.fill: parent
            radius: 20
            color: "transparent"
            border.color: "#F4D6CC"
            border.width: 2
            opacity: 0

            SequentialAnimation on opacity {
                id: pulseAnim
                running: false

                NumberAnimation { to: 0.7; duration: 80 }
                NumberAnimation { to: 0; duration: 300 }
            }
        }

        Connections {
            target: flipAnimation

            function onFinished() {
                pulseAnim.start()
            }
        }
    }
}