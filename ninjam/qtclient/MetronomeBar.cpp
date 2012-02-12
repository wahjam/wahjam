/*
    Copyright (C) 2012 Stefan Hajnoczi <stefanha@gmail.com>

    Based on a demo by Ikkei Shimomura (tea) <Ikkei.Shimomura@gmail.com>

    Wahjam is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Wahjam is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Wahjam; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <QPainter>
#include "MetronomeBar.h"

MetronomeBar::MetronomeBar(QWidget *parent)
  : QWidget(parent)
{
  reset();
}

void MetronomeBar::reset()
{
  bpi = 0;
  beat = 0;
  update();
}

int MetronomeBar::beatsPerInterval() const
{
  return bpi;
}

int MetronomeBar::currentBeat() const
{
  return beat;
}

void MetronomeBar::setBeatsPerInterval(int bpi_)
{
  bpi = bpi_;
  update();
}

void MetronomeBar::setCurrentBeat(int currentBeat)
{
  Q_ASSERT(currentBeat > 0 && currentBeat <= bpi);

  beat = currentBeat;

  /* This could be optimized to repaint only changed regions but it just makes
   * the code trickier.
   */
  update();
}

QSize MetronomeBar::sizeHint() const
{
  return QSize(bpi * 24, 24);
}

QSize MetronomeBar::minimumSizeHint() const
{
  return QSize(bpi * 8, 8);
}

void MetronomeBar::paintEvent(QPaintEvent*)
{
  QPainter painter(this);

  painter.setBrush(Qt::white);
  painter.drawRect(rect());

  // Avoid divide-by-zero when there is nothing to paint
  if (bpi == 0) {
    return;
  }

  qreal beatWidth = width() / (qreal)bpi;
  qreal beatHeight = height();
  qreal padding = 2.0;

  int i;
  for (i = 0; i < beat; i++) {
    QRectF area(i * beatWidth + padding, padding,
                beatWidth - 2 * padding, beatHeight - 2 * padding);
    painter.fillRect(area, Qt::darkBlue);
  }
}
