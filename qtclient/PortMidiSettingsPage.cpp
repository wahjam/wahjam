/*
    Copyright (C) 2013 Stefan Hajnoczi <stefanha@gmail.com>

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

#include <portmidi.h>
#include <QFormLayout>
#include <QLabel>
#include "PortMidiSettingsPage.h"

PortMidiSettingsPage::PortMidiSettingsPage(QWidget *parent)
  : LockableSettingsPage(tr("Disconnect from jam and unlock settings"), parent)
{
  inputDeviceList = new QComboBox;
  inputDeviceList->setEditable(false);
  inputDeviceList->addItem(""); /* no MIDI input */

  outputDeviceList = new QComboBox;
  outputDeviceList->setEditable(false);
  outputDeviceList->addItem(""); /* no MIDI output */

  sendMidiBeatClockBox = new QCheckBox(tr("Send tempo to external apps and devices"));
  sendMidiBeatClockBox->setToolTip(tr("Enable MIDI beat clock"));

  sendMidiStartOnIntervalBox = new QCheckBox(tr("Send MIDI Start on each interval"));
  sendMidiStartOnIntervalBox->setToolTip(tr("Enable to loop external MIDI sequencers"));

  QFormLayout *formLayout = new QFormLayout;
  formLayout->setSpacing(5);
  formLayout->setContentsMargins(2, 2, 2, 2);
  formLayout->addRow(tr("&Input device:"), inputDeviceList);
  formLayout->addRow(tr("&Output device:"), outputDeviceList);
  formLayout->addRow(new QLabel, sendMidiBeatClockBox);
  formLayout->addRow(new QLabel, sendMidiStartOnIntervalBox);
  contentsWidget->setLayout(formLayout);

  populateDeviceLists();
}

void PortMidiSettingsPage::populateDeviceLists()
{
  int i;
  for (i = 0; i < Pm_CountDevices(); i++) {
    const PmDeviceInfo *deviceInfo = Pm_GetDeviceInfo(i);
    if (!deviceInfo) {
      continue;
    }

    if (deviceInfo->input) {
      inputDeviceList->addItem(QString::fromLocal8Bit(deviceInfo->name));
    }
    if (deviceInfo->output) {
      outputDeviceList->addItem(QString::fromLocal8Bit(deviceInfo->name));
    }
  }
}

QString PortMidiSettingsPage::inputDevice() const
{
  return inputDeviceList->currentText();
}

void PortMidiSettingsPage::setInputDevice(const QString &name)
{
  int i = inputDeviceList->findText(name);
  if (i >= 0) {
    inputDeviceList->setCurrentIndex(i);
  }
}

QString PortMidiSettingsPage::outputDevice() const
{
  return outputDeviceList->currentText();
}

void PortMidiSettingsPage::setOutputDevice(const QString &name)
{
  int i = outputDeviceList->findText(name);
  if (i >= 0) {
    outputDeviceList->setCurrentIndex(i);
  }
}

bool PortMidiSettingsPage::sendMidiBeatClock() const
{
  return sendMidiBeatClockBox->isChecked();
}

void PortMidiSettingsPage::setSendMidiBeatClock(bool enable)
{
  sendMidiBeatClockBox->setChecked(enable);
}

bool PortMidiSettingsPage::sendMidiStartOnInterval() const
{
  return sendMidiStartOnIntervalBox->isChecked();
}

void PortMidiSettingsPage::setSendMidiStartOnInterval(bool enable)
{
  sendMidiStartOnIntervalBox->setChecked(enable);
}
