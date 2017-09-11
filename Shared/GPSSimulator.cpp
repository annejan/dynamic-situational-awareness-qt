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

#include "GPSSimulator.h"
#include <QDomDocument>
#include <QXmlStreamReader>
#include <QDomElement>
#include <QTimer>
#include <QStringRef>

using namespace Esri::ArcGISRuntime;

// Default ctor.  To use simulation user must set gpx file the update interval
GPSSimulator::GPSSimulator(QObject* parent) :
  QObject(parent),
  m_gpxReader(new QXmlStreamReader()),
  m_timer(new QTimer(this)),
  m_angleOffset(-180.0, 0.0, 180.0, 0.0) // North-South line used to calculate all headings
{
  connect(m_timer, SIGNAL(timeout()), this, SLOT(handleTimerEvent()));
}

//
// Populates the necessary components to run a gps simulation
//
GPSSimulator::GPSSimulator(const QString& fileName, int updateInterval, QObject* parent) :
  QObject(parent),
  m_gpxReader(new QXmlStreamReader()),
  m_timer(new QTimer(this)),
  m_timerInterval(updateInterval)
{
  connect(m_timer, SIGNAL(timeout()), this, SLOT(handleTimerEvent()));

  if (!setGpxFile(fileName))
  {
    // raise error
    m_gpxFile.setFileName("");
  }
}

//
// dtor
//
GPSSimulator::~GPSSimulator()
{
}

bool GPSSimulator::gotoNextPositionElement()
{
  while (!m_gpxReader->atEnd() && !m_gpxReader->hasError())
  {
    if (m_gpxReader->isStartElement())
    {
      if (m_gpxReader->name().compare(QString("trkpt")) == 0)
      {
        return true;
      }
    }

    m_gpxReader->readNext();
  }

  return false;
}

//
// Point GetNextPoint(QTime&) private method
//   - Convert the current gpx position to Point and QTime parmeters.
//
Point GPSSimulator::getNextPoint(QTime& time)
{
  if (!gotoNextPositionElement())
  {
    return Point();
  }

  // fetch the lat and lon attributes from the trkpt element
  QXmlStreamAttributes attrs = m_gpxReader->attributes();
  const double x = attrs.value("lon").toString().toDouble();
  const double y = attrs.value("lat").toString().toDouble();

  Point point;
  point = Point(x, y, SpatialReference::wgs84());

  // if the new point is the same as the old point then trash it and try to get another.
  if (point == m_latestPoint)
  {
    m_gpxReader->readNext();
    return getNextPoint(time);
  }

  // goto the start of the time child element
  m_gpxReader->readNextStartElement();

  while (m_gpxReader->name().compare(QString("trkpt"), Qt::CaseInsensitive) != 0 && !m_gpxReader->atEnd())
  {
    if (m_gpxReader->isStartElement())
    {
      if (m_gpxReader->name().compare(QString("ele"), Qt::CaseInsensitive) == 0)
      {
        // TODO: do something with the elevation
      }
      else if (m_gpxReader->name().compare(QString("time"), Qt::CaseInsensitive) == 0)
      {
        QString timeString = m_gpxReader->readElementText();
        int hours = timeString.section(":", 0, 0).right(2).toInt();
        int minutes = timeString.section(":", 1, 1).toInt();
        int seconds = timeString.section(":", 2, 2).left(2).toInt();
        time.setHMS(hours ,minutes, seconds, 0);
      }
    }

    m_gpxReader->readNext();
  }

  return point;
}

//
// startSimulation() Public Method:
//   - Loads a GPX file into a stream reader
//   - Fetches the first 3 coordinates
//   - Starts a timer that performs interpolation and position updating
//
void GPSSimulator::startSimulation()
{
  // if the gpx file does not contain enough information to
  // interpolate on then cancel the simulation.
  if (!initializeInterpolationValues())
  {
    return;
  }

  // start the position update timer
  m_timer->start(m_timerInterval);
  m_isStarted = true;
}

void GPSSimulator::pauseSimulation()
{
  m_timer->stop();
}

void GPSSimulator::resumeSimulation()
{
  m_timer->start();
}

bool GPSSimulator::isActive()
{
  return m_timer->isActive();
}

bool GPSSimulator::isStarted()
{
  return m_isStarted;
}

//
// handleTimerEvent() Slot:
//   - increments the current time
//   - fetches new positions from the gpx file as necessary
//   - calculates and sets the current position and orientation
//
void GPSSimulator::handleTimerEvent()
{
  // update the current time
  m_currentTime = m_currentTime.addMSecs(m_timer->interval() * m_playbackMultiplier);

  // determine if a new position needs to be obtained from the gpx
  if (m_currentTime > m_segmentEndTime)
  {
    if (!updateInterpolationParameters())
    {
      m_gpxReader->clear();
      m_gpxReader->addData(m_gpxData);
      initializeInterpolationValues();

      return;
    }
  }

  // normalize the time across the current segment
  const double val1 = static_cast<double>(m_segmentStartTime.msecsTo(m_currentTime));
  const double val2 = static_cast<double>(m_segmentStartTime.msecsTo(m_segmentEndTime));
  const double normalizedTime = val1 / val2;

  // get the interpolated position and orientation on the current
  // segment based on the normalized time.
  const Point currentPosition = normalizedTime <= 0.5 ? m_currentSegment.startPoint() : m_currentSegment.endPoint();
  const double currentOrientation = getInterpolatedOrientation(currentPosition, normalizedTime);

  emit positionUpdateAvailable(currentPosition, currentOrientation);
} // end HandleTimerEvent

//
// Populates all the internal values necessary to start the simulation.
//
bool GPSSimulator::initializeInterpolationValues()
{
  // fetch the first 3 points from the gpx feed to populate the
  // initial interpolation components.
  const Point pt1 = getNextPoint(m_segmentStartTime);
  const Point pt2 = getNextPoint(m_segmentEndTime);
  const Point pt3 = getNextPoint(m_nextSegmentEndTime);

  if (pt1.isEmpty() || pt2.isEmpty() || pt3.isEmpty())
  {
    return false;
  }

  // define the interpolation segments
  m_currentSegment = LineSegment(pt1, pt2, SpatialReference::wgs84());
  m_nextSegment = LineSegment(pt2, pt3, SpatialReference::wgs84());
  m_startHeadingDelta = 0;
  m_endHeadingDelta = heading(m_currentSegment);

  // define the current time as the first timestamp extracted from the gpx file
  m_currentTime = m_segmentStartTime;

  return true;
}

//
// implementation for smooth orientation transfer between segments.
// the smoothing is spread across the final 10% of the current segment
// and the first 10% of the next segment.
//
double GPSSimulator::getInterpolatedOrientation(const Point& currentPosition, double normalizedTime)
{
  LineSegment segment;

  // interpolation of the first 10% of the segment
  if (normalizedTime < 0.1)
  {
    segment = LineSegment(m_currentSegment.startPoint(), currentPosition, SpatialReference::wgs84());
    return heading(m_currentSegment);
  }
  // interpolation of the last 10% of the segment
  else if (normalizedTime > 0.9)
  {
    segment = LineSegment(m_currentSegment.endPoint(), currentPosition, SpatialReference::wgs84());
    return heading(m_currentSegment);
  }

  // no orientation interpolation needed, use the current segments angle
  return heading(m_currentSegment);
}

//
// fetch the next coordinate in the gpx file and updates all the
// internal interpolation vars
//
bool GPSSimulator::updateInterpolationParameters()
{
  m_segmentStartTime = m_segmentEndTime;
  m_segmentEndTime = m_nextSegmentEndTime;
  Point newPt = getNextPoint(m_nextSegmentEndTime);

  // if there are no more points to get then notify simulation to start over
  if (newPt.isEmpty())
  {
    return false;
  }

  // discard the oldest segment and populate the newest segment.
  m_currentSegment = m_nextSegment;
  m_nextSegment = LineSegment(m_currentSegment.endPoint(), newPt, SpatialReference::wgs84());

  // discard the oldest orientation delta and populate the newest
  m_startHeadingDelta = m_endHeadingDelta;
  m_endHeadingDelta = heading(m_currentSegment);

  return true;
}

//
// getter for the gpx file location
//
QString GPSSimulator::gpxFile()
{
  return m_gpxFile.fileName();
}

//
// setter for the gpx file location
//
bool GPSSimulator::setGpxFile(const QString& fileName)
{
  if (!QFile::exists(fileName))
    return false;

  if (m_gpxFile.isOpen())
    m_gpxFile.close();

  m_gpxFile.setFileName(fileName);

  if (!m_gpxFile.open(QFile::ReadOnly | QFile::Text))
    return false;

  m_gpxData = m_gpxFile.readAll();
  m_gpxReader->clear();
  m_gpxReader->addData(m_gpxData);
  m_gpxFile.close();

  m_isStarted = false;

  return true;
}

//
// getter for the simulation timers's polling interval
//
int GPSSimulator::timerInterval()
{
  return m_timerInterval;
}

//
// setter for the simulation timers's polling interval
//
void GPSSimulator::setTimerInterval(int ms)
{
  m_timerInterval = ms;
}

//
// getter for the playback multiplier
//
int GPSSimulator::playbackMultiplier()
{
  return m_playbackMultiplier;
}

//
// setter for the playback modifier.  Used if
// gpx timestamps are either too close or two far
//
void GPSSimulator::setPlaybackMultiplier(int val)
{
  m_playbackMultiplier = val;
}

double GPSSimulator::heading(const Esri::ArcGISRuntime::LineSegment& segment) const
{
  const auto startPoint = segment.startPoint();
  const auto endPoint = segment.endPoint();
  return QLineF(startPoint.x(), startPoint.y(), endPoint.x(), endPoint.y()).angleTo(m_angleOffset);
}
