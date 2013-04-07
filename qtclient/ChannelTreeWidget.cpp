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
  ChannelIndexRole = Qt::UserRole,
  UserIndexRole,
};

ChannelTreeWidget::ChannelTreeWidget(QWidget *parent)
  : QTreeWidget(parent)
{
  setHeaderLabels(QStringList("Name") << "Mute");
  setRootIsDecorated(false);
  setItemsExpandable(false);
  setSelectionMode(QAbstractItemView::NoSelection);

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

QTreeWidgetItem *ChannelTreeWidget::addChannelItem(QTreeWidgetItem *parent, const QString &text)
{
  QTreeWidgetItem *channel = new QTreeWidgetItem(parent);

  channel->setText(0, text);
  channel->setCheckState(1, Qt::Unchecked);
  return channel;
}

void ChannelTreeWidget::handleItemChanged(QTreeWidgetItem *item, int column)
{
  bool state = item->data(column, Qt::CheckStateRole).toBool();
  int useridx = item->data(0, UserIndexRole).toInt(NULL);
  int channelidx = item->data(0, ChannelIndexRole).toInt(NULL);
  if (column == 1) {
    emit RemoteChannelMuteChanged(useridx, channelidx, state);
  }
}

ChannelTreeWidget::RemoteChannelUpdater::RemoteChannelUpdater(ChannelTreeWidget *owner_)
  : owner(owner_), toplevelidx(-1), childidx(-1)
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
    channel = owner->addChannelItem(user, name);
  }

  channel->setData(0, UserIndexRole, useridx);
  channel->setData(0, ChannelIndexRole, channelidx);
  channel->setCheckState(1, mute ? Qt::Checked : Qt::Unchecked);
}

/* Delete unused channels from previous user */
void ChannelTreeWidget::RemoteChannelUpdater::prunePreviousUser()
{
  if (toplevelidx == -1) {
    return;
  }

  QTreeWidgetItem *user = owner->topLevelItem(toplevelidx);
  while (user->childCount() > childidx + 1) {
    QTreeWidgetItem *channel = user->takeChild(user->childCount() - 1);
    delete channel;
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
