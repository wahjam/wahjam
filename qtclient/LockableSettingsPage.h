/*
    Copyright (C) 2020 Stefan Hajnoczi <stefanha@jammr.net>

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

#ifndef _LOCKABLESETTINGSPAGE_H_
#define _LOCKABLESETTINGSPAGE_H_

#include <QPushButton>
#include <QWidget>

/* A settings dialog page that can be "locked" to disable changes. The user can
 * click an "unlock" button to enable changes.
 *
 * Derived classes must add their layout or widgets to contentsWidget.
 */
class LockableSettingsPage : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(bool locked WRITE setLocked)

public:
  LockableSettingsPage(const QString &unlockButtonText, QWidget *parent = 0);
  virtual ~LockableSettingsPage() {}
  void setLocked(bool locked);

signals:
  /* User wishes to unlock settings */
  void unlockRequest();

protected:
  QWidget *contentsWidget;

private:
  QPushButton *unlockButton;
};

#endif /* _LOCKABLESETTINGSPAGE_H_ */
