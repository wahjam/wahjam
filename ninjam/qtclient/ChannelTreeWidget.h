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

  void addLocalChannel(int ch, const QString &name, bool mute);

  /*
   * Remote user and channel updates must be performed by enumerating all users
   * and their channels each time.  Any users or channels present in the last
   * update but not in the current update will be removed.
   *
   * This interface is an artifact of how NJClient only informs us that user
   * information has changed, but not what specifically to add/remove/update.
   */
  class RemoteChannelUpdater
  {
  public:
    RemoteChannelUpdater(ChannelTreeWidget *owner);
    void addUser(int useridx, const QString &name);
    void addChannel(int channelidx, const QString &name, bool mute);
    void commit();

  private:
    ChannelTreeWidget *owner;
    int toplevelidx;
    int childidx;
    int useridx;

    void prunePreviousUser();
  };

signals:
  void MetronomeMuteChanged(bool mute);
  void LocalChannelMuteChanged(int ch, bool mute);
  void RemoteChannelMuteChanged(int useridx, int channelidx, bool mute);

private slots:
  void handleItemChanged(QTreeWidgetItem *item, int column);

private:
  QTreeWidgetItem *addRootItem(const QString &text);
  QTreeWidgetItem *addChannelItem(QTreeWidgetItem *parent, const QString &text);
};

#endif /* _CHANNELTREEWIDGET_H */
