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

#ifndef _CHANNELTREEWIDGET_H
#define _CHANNELTREEWIDGET_H

#include <QTreeWidget>

class ChannelTreeWidget : public QTreeWidget
{
  Q_OBJECT

public:
  ChannelTreeWidget(QWidget *parent = 0);

signals:
  void MetronomeMuteChanged(bool mute);
  void MetronomeBoostChanged(bool boost);

private slots:
  void handleItemChanged(QTreeWidgetItem *item, int column);

private:
  enum ChannelFlags {
    CF_BROADCAST = 0x1,
    CF_BOOST     = 0x2,
  };

  QTreeWidgetItem *addRootItem(const QString &text);
  QTreeWidgetItem *addChannelItem(QTreeWidgetItem *parent, const QString &text, int flags);
};

#endif /* _CHANNELTREEWIDGET_H */
