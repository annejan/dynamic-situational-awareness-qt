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

#ifndef FEATUREQUERYRESULTSMANAGER_H
#define FEATUREQUERYRESULTSMANAGER_H

#include <QList>

namespace Esri
{
namespace ArcGISRuntime
{
  class FeatureQueryResult;
}
}

struct FeatureQueryResultManager {

  Esri::ArcGISRuntime::FeatureQueryResult* m_results;

  FeatureQueryResultManager(Esri::ArcGISRuntime::FeatureQueryResult* results);

  ~FeatureQueryResultManager();
};

#endif // FEATUREQUERYRESULTSMANAGER_H
