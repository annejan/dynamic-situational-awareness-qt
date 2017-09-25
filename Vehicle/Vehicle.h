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

#ifndef VEHICLE_H
#define VEHICLE_H

namespace Esri
{
namespace ArcGISRuntime
{
class SceneQuickView;
namespace Toolkit
{
class CoordinateConversionController;
}
}
}

#include <QQuickItem>

class DsaController;

class Vehicle : public QQuickItem
{
  Q_OBJECT

public:
  Vehicle(QQuickItem* parent = nullptr);
  ~Vehicle();

  void componentComplete() override;

private:
  void setCoordinateConversionOptions();
  Esri::ArcGISRuntime::Toolkit::CoordinateConversionController* m_coordinateConversionController = nullptr;
  Esri::ArcGISRuntime::Scene*             m_scene = nullptr;
  Esri::ArcGISRuntime::SceneQuickView*    m_sceneView = nullptr;
  QString                                 m_dataPath;
  Esri::ArcGISRuntime::SceneQuickView*    m_sceneView = nullptr;
  DsaController*                          m_controller = nullptr;
};

#endif // VEHICLE_H
