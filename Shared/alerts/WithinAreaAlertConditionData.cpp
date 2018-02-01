// Copyright 2017 ESRI
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

#include "AlertSource.h"
#include "AlertSpatialTarget.h"
#include "WithinAreaAlertConditionData.h"

#include "GeoElement.h"
#include "GeometryEngine.h"
#include "Point.h"

using namespace Esri::ArcGISRuntime;

WithinAreaAlertConditionData::WithinAreaAlertConditionData(const QString& name,
                                                           AlertLevel level,
                                                           AlertSource* source,
                                                           AlertSpatialTarget* target,
                                                            QObject* parent):
  AlertConditionData(name, level, source, target, parent),
  m_spatialTarget(target)
{

}

WithinAreaAlertConditionData::~WithinAreaAlertConditionData()
{

}

AlertSpatialTarget* WithinAreaAlertConditionData::spatialTarget() const
{
  return m_spatialTarget;
}

bool WithinAreaAlertConditionData::matchesQuery() const
{
  Geometry sourceWgs84 = GeometryEngine::project(sourceLocation(), SpatialReference::wgs84());
  const QList<Geometry> targetGeometries = spatialTarget()->targetGeometries(sourceWgs84.extent());

  for (const Geometry& target : targetGeometries)
  {
    if (target.geometryType() != GeometryType::Polygon)
      continue;

    const Geometry targetWgs84 = GeometryEngine::project(target, sourceWgs84.spatialReference());
    if (GeometryEngine::instance()->intersects(sourceWgs84, targetWgs84))
      return true;
  }

  return false;
}
