/*
**  This file is part of Vidalia, and is subject to the license terms in the
**  LICENSE file, found in the top level directory of this distribution. If you
**  did not receive the LICENSE file with this file, you may obtain it from the
**  Vidalia source package distributed by the Vidalia Project at
**  http://www.torproject.org/projects/vidalia.html. No part of Vidalia,
**  including this file, may be copied, modified, propagated, or distributed
**  except according to the terms described in the LICENSE file.
*/

/*
** \file GraphFrame.h
** \brief Graphs a series of send and receive data points
*/

#include <QtGui>
#include "BandwidthIcon.h"


BandwidthIcon::BandwidthIcon(QObject *parent)
{
  /* Create Graph Frame related objects */
  _recvData = new QList<qreal>();
  _sendData = new QList<qreal>();
  _painter = new QPainter();
  _graphStyle = SolidLine;

  /* Initialize graph values */
  _recvData->prepend(0);
  _sendData->prepend(0);
  _maxPoints = getNumPoints();
  _maxPosition = 0;
  _showRecv = true;
  _showSend = true;
  _maxValue = MIN_SCALE;
  _scaleWidth = 0;
}

/** Gets the width of the desktop, which is the maximum number of points
 * we can plot in the graph. */
int
BandwidthIcon::getNumPoints()
{
  return 22 - _scaleWidth;
}

void
BandwidthIcon::paintEvent(){
  QPixmap mask(22,22);
  mask.fill(Qt::white);

  QPixmap mask2( ":/images/22x22/tor-on.png");
  //_rec = mask.rect();
  _rec = QRect(0,0,21,21);
  /* Start the painter */
  _painter->begin(&mask);

  /* We want antialiased lines and text */
  _painter->setRenderHint(QPainter::Antialiasing);
  _painter->setRenderHint(QPainter::TextAntialiasing);

  /* Fill in the background */
  _painter->fillRect(_rec, QBrush(BACK_COLOR));
  _painter->drawRect(_rec);

  
  /* Plot the send/receive data */
  paintData();
  

  /* Stop the painter */
  _painter->end();

  _trayIcon.setIcon(mask);
  _trayIcon.show();

}


/** Returns a list of points on the bandwidth graph based on the supplied set
 * of send or receive values. */
QVector<QPointF>
BandwidthIcon::pointsFromData(QList<qreal>* list)
{
  QVector<QPointF> points;
  int x = _rec.width();
  int y = _rec.height();
  qreal scale = (y - (y/10)) / _maxValue;
  qreal currValue;

  /* Translate all data points to points on the graph frame */
  points << QPointF(x, y);
  for (int i = 0; i < list->size(); i++) {
    currValue = y - (list->at(i) * scale);
    if (x - SCROLL_STEP < _scaleWidth) {
      points << QPointF(_scaleWidth, currValue);
      break;
    }
    points << QPointF(x, currValue);
    x -= SCROLL_STEP;
  }
  points << QPointF(_scaleWidth, y);
  return points;
}

/** Plots an integral using the data points in <b>points</b>. The area will be
 * filled in using <b>color</b> and an alpha-blending level of <b>alpha</b>
 * (default is opaque). */
void
BandwidthIcon::paintIntegral(QVector<QPointF> points, QColor color, qreal alpha)
{
  /* Save the current brush, plot the integral, and restore the old brush */
  QBrush oldBrush = _painter->brush();
  color.setAlphaF(alpha);
  _painter->setBrush(QBrush(color));
  _painter->drawPolygon(points.data(), points.size());
  _painter->setBrush(oldBrush);
}

/** Iterates the input list and draws a line on the graph in the appropriate
 * color. */
void
BandwidthIcon::paintLine(QVector<QPointF> points, QColor color, Qt::PenStyle lineStyle)
{
  /* Save the current brush, plot the line, and restore the old brush */
  QPen oldPen = _painter->pen();
  _painter->setPen(QPen(color, lineStyle));
  _painter->drawPolyline(points.data(), points.size());
  _painter->setPen(oldPen);
}


void
BandwidthIcon::paintData()
{
  QVector<QPointF> recvPoints, sendPoints;

  /* Convert the bandwidth data points to graph points */
  recvPoints = pointsFromData(_recvData);
  sendPoints = pointsFromData(_sendData);

  if (_graphStyle == AreaGraph) {
    /* Plot the bandwidth data as area graphs */
    if (_showRecv)
      paintIntegral(recvPoints, RECV_COLOR, 0.6);
    if (_showSend)
      paintIntegral(sendPoints, SEND_COLOR, 0.4);
  }

  /* Plot the bandwidth as solid lines. If the graph style is currently an
   * area graph, we end up outlining the integrals. */
  if (_showRecv)
    paintLine(recvPoints, RECV_COLOR);
  if (_showSend)
    paintLine(sendPoints, SEND_COLOR);
}

/** Adds new data points to the graph. */
void
BandwidthIcon::addPoints(qreal recv, qreal send)
{
  /* If maximum number of points plotted, remove oldest */
  if (_sendData->size() == _maxPoints) {
    _sendData->removeLast();
    _recvData->removeLast();
  }

  /* Update the displayed maximum */
  if (_maxPosition >= _maxPoints) {
    _maxValue = MIN_SCALE;
    foreach(qreal send, *_sendData)
      if(send > _maxValue)
        _maxValue = send;
    foreach(qreal recv, *_recvData)
      if(recv > _maxValue)
        _maxValue = recv;
    _maxPosition = 0;
  }

  /* Add the points to their respective lists */
  _sendData->prepend(send);
  _recvData->prepend(recv);

  /* Add to the total counters */
  _totalSend += send;
  _totalRecv += recv;

  bool maxUpdated = false;
  /* Check for a new maximum value */
  if (send > _maxValue) {
    _maxValue = send;
    maxUpdated = true;
  }

  if (recv > _maxValue) {
    _maxValue = recv;
    maxUpdated = true;
  }

  if (maxUpdated) {
    _maxPosition = 0;
  } else {
    _maxPosition++;
  }

  paintEvent();
}

