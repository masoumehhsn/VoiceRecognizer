import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Window 2.15

Window {
    width: animation.width
    height: animation.height
    visible: true
    title: qsTr("STT")
    Connections {
        target: STTHandler
        function onStartFullRecordingSignal() {
            animation.playing = true
        }
        function onStopFullRecordingSignal() {
            animation.playing = false
        }
    }
    AnimatedImage {
        id: animation;
        source: "qrc:/img/microphone3.gif"
        playing : false
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width * 0.8
            height: parent.height * 0.07
            radius: 12
            color: "white"
            border.width: 1
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 20
            TextField {
                anchors.fill: parent
                anchors.margins: 2
                text: STTHandler.command
                verticalAlignment: Text.AlignVCenter
                background: null
            }
        }



    }


}
