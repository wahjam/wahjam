/*
    Copyright (C) 2013-2020 Stefan Hajnoczi <stefanha@gmail.com>

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

#ifndef _PORTMIDISETTINGSPAGE_H_
#define _PORTMIDISETTINGSPAGE_H_

#include <QComboBox>
#include <QCheckBox>
#include "LockableSettingsPage.h"

class PortMidiSettingsPage : public LockableSettingsPage
{
  Q_OBJECT
  Q_PROPERTY(QString inputDevice READ inputDevice WRITE setInputDevice)
  Q_PROPERTY(QString outputDevice READ outputDevice WRITE setOutputDevice)
  Q_PROPERTY(bool sendMidiBeatClock READ sendMidiBeatClock WRITE setSendMidiBeatClock)

public:
  PortMidiSettingsPage(QWidget *parent = 0);
  QString inputDevice() const;
  void setInputDevice(const QString &name);
  QString outputDevice() const;
  void setOutputDevice(const QString &name);
  bool sendMidiBeatClock() const;
  void setSendMidiBeatClock(bool enable);

private:
  QComboBox *inputDeviceList;
  QComboBox *outputDeviceList;
  QCheckBox *sendMidiBeatClockBox;

  void populateDeviceLists();
};

#endif /* _PORTMIDISETTINGSPAGE_H_ */
