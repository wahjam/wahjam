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

#include <portaudio.h>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "PortAudioConfigDialog.h"

PortAudioConfigDialog::PortAudioConfigDialog(QWidget *parent)
  : QDialog(parent)
{
  inputDeviceList = new QComboBox;
  inputDeviceList->setEditable(false);

  outputDeviceList = new QComboBox;
  outputDeviceList->setEditable(false);

  hostAPIList = new QComboBox;
  hostAPIList->setEditable(false);
  connect(hostAPIList, SIGNAL(currentIndexChanged(int)),
          this, SLOT(hostAPIIndexChanged(int)));

  QPushButton *applyButton = new QPushButton(tr("&Apply"));
  connect(applyButton, SIGNAL(clicked()), this, SLOT(accept()));

  QPushButton *cancelButton = new QPushButton(tr("&Cancel"));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

  QVBoxLayout *vlayout = new QVBoxLayout;
  QFormLayout *formLayout = new QFormLayout;
  formLayout->setSpacing(5);
  formLayout->setContentsMargins(2, 2, 2, 2);
  formLayout->addRow(tr("Audio &system:"), hostAPIList);
  formLayout->addRow(tr("&Input device:"), inputDeviceList);
  formLayout->addRow(tr("&Output device:"), outputDeviceList);
  vlayout->addLayout(formLayout);
  QHBoxLayout *hlayout = new QHBoxLayout;
  hlayout->setSpacing(0);
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->addWidget(applyButton);
  hlayout->addWidget(cancelButton);
  vlayout->setSpacing(2);
  vlayout->setContentsMargins(5, 5, 5, 5);
  vlayout->addLayout(hlayout);
  setLayout(vlayout);
  setWindowTitle(tr("Configure audio devices..."));

  populateHostAPIList();
}

void PortAudioConfigDialog::populateHostAPIList()
{
  PaHostApiIndex i;
  for (i = 0; i < Pa_GetHostApiCount(); i++) {
    const PaHostApiInfo *hostAPIInfo = Pa_GetHostApiInfo(i);
    if (hostAPIInfo) {
      QString name = QString::fromLocal8Bit(hostAPIInfo->name);
      hostAPIList->addItem(name, i);
    }
  }
}

void PortAudioConfigDialog::hostAPIIndexChanged(int index)
{
  inputDeviceList->clear();
  outputDeviceList->clear();

  if (index < 0) {
    return;
  }

  PaHostApiIndex apiIndex = hostAPIList->itemData(index).toInt(NULL);
  const PaHostApiInfo *hostAPIInfo = Pa_GetHostApiInfo(apiIndex);
  if (!hostAPIInfo) {
    return;
  }

  int i;
  for (i = 0; i < hostAPIInfo->deviceCount; i++) {
    PaDeviceIndex devIndex = Pa_HostApiDeviceIndexToDeviceIndex(apiIndex, i);
    const PaDeviceInfo *devInfo = Pa_GetDeviceInfo(devIndex);
    QString name = QString::fromLocal8Bit(devInfo->name);

    if (devInfo->maxInputChannels > 0) {
      inputDeviceList->addItem(name, devIndex);
    }
    if (devInfo->maxOutputChannels > 0) {
      outputDeviceList->addItem(name, devIndex);
    }
  }
}

QString PortAudioConfigDialog::hostAPI() const
{
  return hostAPIList->currentText();
}

void PortAudioConfigDialog::setHostAPI(const QString &name)
{
  int i = hostAPIList->findText(name);
  if (i >= 0) {
    hostAPIList->setCurrentIndex(i);
  }
}

QString PortAudioConfigDialog::inputDevice() const
{
  return inputDeviceList->currentText();
}

void PortAudioConfigDialog::setInputDevice(const QString &name)
{
  int i = inputDeviceList->findText(name);
  if (i >= 0) {
    inputDeviceList->setCurrentIndex(i);
  }
}

QString PortAudioConfigDialog::outputDevice() const
{
  return outputDeviceList->currentText();
}

void PortAudioConfigDialog::setOutputDevice(const QString &name)
{
  int i = outputDeviceList->findText(name);
  if (i >= 0) {
    outputDeviceList->setCurrentIndex(i);
  }
}
