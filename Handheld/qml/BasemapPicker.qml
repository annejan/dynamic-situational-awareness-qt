
// Copyright 2016 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import Esri.Handheld 1.0

Item {
    signal basemapSelected();

    property real scaleFactor: 1.

    BasemapPickerController {
        id: toolController
    }

    ListView {
        id: basemapsList

        anchors{
            margins: 32 * scaleFactor
            fill: parent
        }

        spacing: 32 * scaleFactor

        clip: true
        model: toolController.basemapsModel

        delegate: Item {

            width: basemapsList.width
            height: 64 * scaleFactor

            Button {
                id: button
                anchors {
                    margins: 8 * scaleFactor
                    horizontalCenter: parent.horizontalCenter
                }

                height: parent.height

                text: display

                onClicked: {
                    toolController.basemapNameSelected(display);
                    basemapSelected();
                }
            }

            Image {
                anchors.left: button.right
                anchors.top: button.top
                anchors.bottom: button.bottom
                anchors.margins: 8 * scaleFactor

                source: toolController.imageForBasemap(display)
                fillMode: Image.PreserveAspectFit
            }
        }
    }

    Component.onCompleted: toolController.basemapNameSelected("LightGreyCanvas");
}
