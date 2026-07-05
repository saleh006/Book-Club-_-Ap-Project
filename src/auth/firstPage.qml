import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

Item {
    id: root
    width: 800
    height: 500

    Rectangle {
        anchors.fill: parent
        color: "#060508"
    }

    Item {
        anchors.fill: parent
        opacity: 0.7

        Rectangle {
            width: 4; height: 4; radius: 2; color: "#FFEAD2"
            x: parent.width * 0.60; y: parent.height * 0.2
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.2; to: 1.0; duration: 1800; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 1.0; to: 0.2; duration: 1800; easing.type: Easing.InOutQuad }
            }
        }
        Rectangle {
            width: 5; height: 5; radius: 2.5; color: "#7C3E66"
            x: parent.width * 0.88; y: parent.height * 0.4
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.3; to: 0.9; duration: 2200; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 0.9; to: 0.3; duration: 2200; easing.type: Easing.InOutQuad }
            }
        }
        Rectangle {
            width: 3; height: 3; radius: 1.5; color: "#FFEAD2"
            x: parent.width * 0.75; y: parent.height * 0.75
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.1; to: 1.0; duration: 1300; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 1.0; to: 0.1; duration: 1300; easing.type: Easing.InOutQuad }
            }
        }
        Rectangle {
            width: 4; height: 4; radius: 2; color: "#A594B3"
            x: parent.width * 0.55; y: parent.height * 0.6
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.2; to: 0.8; duration: 2000; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 0.8; to: 0.2; duration: 2000; easing.type: Easing.InOutQuad }
            }
        }

        Rectangle {
            width: 6; height: 6; radius: 3; color: "#FFEAD2"
            x: parent.width * 0.5; y: parent.height * 0.8
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.2; to: 1.0; duration: 2000; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 1.0; to: 0.2; duration: 2000; easing.type: Easing.InOutQuad }
            }
        }
    }

    Component.onCompleted: {
        panelAnimation.start()
        contentAnimation.start()
    }
    ParallelAnimation {
        id: panelAnimation
        NumberAnimation { target: leftSideContainer; property: "opacity"; from: 0; to: 1; duration: 900; easing.type: Easing.OutCubic }
        NumberAnimation { target: leftSideContainer; property: "scale"; from: 0.95; to: 1.0; duration: 900; easing.type: Easing.OutBack }
    }
    ParallelAnimation {
        id: contentAnimation
        NumberAnimation { target: rightSideContent; property: "opacity"; from: 0; to: 1; duration: 1000; easing.type: Easing.OutCubic }
        NumberAnimation { target: rightSideContent; property: "y"; from: (root.height / 2) + 40; to: (root.height / 2); duration: 1000; easing.type: Easing.OutCubic }
    }

    RowLayout {
        id: mainRowLayout
        anchors.fill: parent
        anchors.margins: 40
        spacing: 50
        Item {
            id: leftSideContainer
            Layout.preferredWidth: parent.width * 0.45
            Layout.fillHeight: true
            opacity: 0
            Rectangle {
                id: leftPanel
                anchors.fill: parent
                radius: 24
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#1A0F1B" }
                    GradientStop { position: 1.0; color: "#060508" }
                }
                Rectangle {
                    id: gifMaskContainer
                    anchors.centerIn: parent
                    width: parent.width * 0.9
                    height: parent.height * 0.9
                    radius: 20
                    color: "#060508"
                    clip: true
                    AnimatedImage {
                        id: backgroundGif
                        source: "file:///D:/Book-Club-_-Ap-Project/images/gif.gif"
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                        playing: true
                        cache: false
                    }
                }
            }
            MultiEffect {
                anchors.fill: leftPanel
                source: leftPanel
                shadowEnabled: true
                shadowVerticalOffset: 15
                shadowBlur: 1.2
                shadowColor: "#80000000"
                z: -1
            }
        }
        Item { id: rightSideContent }
    }
}