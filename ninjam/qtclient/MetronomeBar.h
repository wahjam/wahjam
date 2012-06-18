/*
    Copyright (C) 2012 Stefan Hajnoczi <stefanha@gmail.com>

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

#ifndef _METRONOMEBAR_H_
#define _METRONOMEBAR_H_

#include <QFrame>

/**
 * A custom widget to show beats
 *
 * Beats are grouped into intervals.  Each interval has a fixed number of
 * beats.  This widget shows the current beat similar to a progress bar widget.
 */
class MetronomeBar : public QFrame
{
  Q_OBJECT
  Q_PROPERTY(int beatsPerInterval READ beatsPerInterval WRITE setBeatsPerInterval)
  Q_PROPERTY(int currentBeat READ currentBeat WRITE setCurrentBeat)

public:
  MetronomeBar(QWidget *parent = 0);

  int beatsPerInterval() const;
  int currentBeat() const;

  QSize sizeHint() const;
  QSize minimumSizeHint() const;

public slots:
  void reset();
  void setBeatsPerInterval(int bpi);
  void setCurrentBeat(int currentBeat);

private:
  void paintEvent(QPaintEvent *event);

  int bpi;
  int beat;
};

#endif /* _METRONOMEBAR_H_ */
