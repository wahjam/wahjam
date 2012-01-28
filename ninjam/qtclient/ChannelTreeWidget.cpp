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

#include <QCheckBox>
#include "ChannelTreeWidget.h"

enum
{
  ItemTypeMetronome = 0,

  ItemTypeRole = Qt::UserRole,
};

ChannelTreeWidget::ChannelTreeWidget(QWidget *parent)
  : QTreeWidget(parent)
{
  setHeaderLabels(QStringList("Name") << "Mute" << "Broadcast" << "Boost (+3dB)");
  setRootIsDecorated(false);
  setItemsExpandable(false);
  setSelectionMode(QAbstractItemView::NoSelection);

  QTreeWidgetItem *local = addRootItem("Local");
  QTreeWidgetItem *metronome = addChannelItem(local, "Metronome", CF_BOOST);
  metronome->setData(0, ItemTypeRole, ItemTypeMetronome);

  connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
          this, SLOT(handleItemChanged(QTreeWidgetItem*, int)));
}

QTreeWidgetItem *ChannelTreeWidget::addRootItem(const QString &text)
{
  QTreeWidgetItem *item = new QTreeWidgetItem(this);
  item->setText(0, text);
  item->setExpanded(true);
  return item;
}

QTreeWidgetItem *ChannelTreeWidget::addChannelItem(QTreeWidgetItem *parent, const QString &text, int flags)
{
  QTreeWidgetItem *channel = new QTreeWidgetItem(parent);

  channel->setText(0, text);
  channel->setCheckState(1, Qt::Unchecked);
  if (flags & CF_BROADCAST) {
    channel->setCheckState(2, Qt::Unchecked);
  }
  if (flags & CF_BOOST) {
    channel->setCheckState(3, Qt::Unchecked);
  }
  return channel;
}

void ChannelTreeWidget::handleItemChanged(QTreeWidgetItem *item, int column)
{
  QVariant itemType = item->data(0, ItemTypeRole);
  if (!itemType.isValid()) {
    return;
  }

  bool state = item->data(column, Qt::CheckStateRole).toBool();
  switch (itemType.toInt(NULL)) {
  case ItemTypeMetronome:
    if (column == 1) {
      emit MetronomeMuteChanged(state);
    } else if (column == 3) {
      emit MetronomeBoostChanged(state);
    }
    break;
  }
}
