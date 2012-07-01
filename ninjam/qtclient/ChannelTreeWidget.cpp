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
  ItemTypeLocalChannel,
  ItemTypeRemoteChannel,

  ItemTypeRole = Qt::UserRole,
  ChannelIndexRole,
  UserIndexRole,
};

ChannelTreeWidget::ChannelTreeWidget(QWidget *parent)
  : QTreeWidget(parent)
{
  setHeaderLabels(QStringList("Name") << "Mute" << "Broadcast");
  setRootIsDecorated(false);
  setItemsExpandable(false);
  setSelectionMode(QAbstractItemView::NoSelection);

  QTreeWidgetItem *local = addRootItem("Local");
  QTreeWidgetItem *metronome = addChannelItem(local, "Metronome", 0);
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
  return channel;
}

void ChannelTreeWidget::addLocalChannel(int ch, const QString &name, bool mute, bool broadcast)
{
  QTreeWidgetItem *local = topLevelItem(0);
  QTreeWidgetItem *channel = addChannelItem(local, name, CF_BROADCAST);

  channel->setData(0, ItemTypeRole, ItemTypeLocalChannel);
  channel->setData(0, ChannelIndexRole, ch);
  channel->setCheckState(1, mute ? Qt::Checked : Qt::Unchecked);
  channel->setCheckState(2, broadcast ? Qt::Checked : Qt::Unchecked);
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
    }
    break;
  case ItemTypeLocalChannel:
  {
    int ch = item->data(0, ChannelIndexRole).toInt(NULL);
    if (column == 1) {
      emit LocalChannelMuteChanged(ch, state);
    } else if (column == 2) {
      emit LocalChannelBroadcastChanged(ch, state);
    }
    break;
  }
  case ItemTypeRemoteChannel:
  {
    int useridx = item->data(0, UserIndexRole).toInt(NULL);
    int channelidx = item->data(0, ChannelIndexRole).toInt(NULL);
    if (column == 1) {
      emit RemoteChannelMuteChanged(useridx, channelidx, state);
    }
    break;
  }
  }
}

ChannelTreeWidget::RemoteChannelUpdater::RemoteChannelUpdater(ChannelTreeWidget *owner_)
  : owner(owner_), toplevelidx(0), childidx(-1)
{
}

void ChannelTreeWidget::RemoteChannelUpdater::addUser(int useridx_, const QString &name)
{
  prunePreviousUser();

  QTreeWidgetItem *item = owner->topLevelItem(++toplevelidx);
  if (item) {
    item->setText(0, name);
  } else {
    owner->addRootItem(name);
  }
  childidx = -1;
  useridx = useridx_;
}

void ChannelTreeWidget::RemoteChannelUpdater::addChannel(int channelidx, const QString &name, bool mute)
{
  QTreeWidgetItem *user = owner->topLevelItem(toplevelidx);
  QTreeWidgetItem *channel = user->child(++childidx);
  if (channel) {
    channel->setText(0, name);
  } else {
    channel = owner->addChannelItem(user, name, 0);
  }

  channel->setData(0, ItemTypeRole, ItemTypeRemoteChannel);
  channel->setData(0, UserIndexRole, useridx);
  channel->setData(0, ChannelIndexRole, channelidx);
  channel->setCheckState(1, mute ? Qt::Checked : Qt::Unchecked);
}

/* Delete unused channels from previous user */
void ChannelTreeWidget::RemoteChannelUpdater::prunePreviousUser()
{
  if (toplevelidx > 0) {
    QTreeWidgetItem *user = owner->topLevelItem(toplevelidx);
    while (user->childCount() > childidx + 1) {
      QTreeWidgetItem *channel = user->takeChild(user->childCount() - 1);
      delete channel;
    }
  }
}

void ChannelTreeWidget::RemoteChannelUpdater::commit()
{
  prunePreviousUser();

  while (owner->topLevelItemCount() > toplevelidx + 1) {
    QTreeWidgetItem *user = owner->takeTopLevelItem(owner->topLevelItemCount() - 1);
    while (user->childCount() > 0) {
      QTreeWidgetItem *channel = user->takeChild(user->childCount() - 1);
      delete channel;
    }
    delete user;
  }
}
