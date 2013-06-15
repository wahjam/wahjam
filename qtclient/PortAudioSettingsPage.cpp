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

#include <math.h>
#include <portaudio.h>
#include <QIcon>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include "PortAudioSettingsPage.h"

PortAudioSettingsPage::PortAudioSettingsPage(QWidget *parent)
  : QWidget(parent), validateSettingsEntryCount(0)
{
  inputDeviceList = new QComboBox;
  inputDeviceList->setEditable(false);
  connect(inputDeviceList, SIGNAL(currentIndexChanged(int)),
          this, SLOT(deviceIndexChanged(int)));

  unmuteLocalChannelsBox = new QCheckBox(tr("Play back my audio"));
  unmuteLocalChannelsBox->setToolTip(tr("Disable if you play an acoustic instrument"));

  outputDeviceList = new QComboBox;
  outputDeviceList->setEditable(false);
  connect(outputDeviceList, SIGNAL(currentIndexChanged(int)),
          this, SLOT(deviceIndexChanged(int)));

  QLabel *sadLabel = new QLabel;
  sadLabel->setPixmap(QIcon::fromTheme("face-sad").pixmap(16));
  QLabel *invalidSettingsLabel = new QLabel(tr("<b><font color=\"red\">Sound devices do not support these settings</font></b>"));
  QHBoxLayout *invalidSettingsLayout = new QHBoxLayout;
  invalidSettingsLayout->addStretch(1);
  invalidSettingsLayout->addWidget(sadLabel);
  invalidSettingsLayout->addWidget(invalidSettingsLabel);
  invalidSettingsWidget = new QWidget;
  invalidSettingsWidget->setLayout(invalidSettingsLayout);

  QLabel *smileyLabel = new QLabel;
  smileyLabel->setPixmap(QIcon::fromTheme("face-smile").pixmap(16));
  QLabel *validSettingsLabel = new QLabel(tr("<b><font color=\"green\">Sound devices support these settings</font></b>"));
  QHBoxLayout *validSettingsLayout = new QHBoxLayout;
  validSettingsLayout->addStretch(1);
  validSettingsLayout->addWidget(smileyLabel);
  validSettingsLayout->addWidget(validSettingsLabel);
  validSettingsWidget = new QWidget;
  validSettingsWidget->setLayout(validSettingsLayout);

  sampleRateList = new QComboBox;
  sampleRateList->setEditable(false);
  sampleRateList->addItems(QStringList() << "32000" << "44100" << "48000" << "88200" << "96000");
  connect(sampleRateList, SIGNAL(currentIndexChanged(int)),
          this, SLOT(sampleRateIndexChanged(int)));

  latencyList = new QComboBox();
  connect(latencyList, SIGNAL(currentIndexChanged(int)),
          this, SLOT(latencyIndexChanged(int)));

  hostAPIList = new QComboBox;
  hostAPIList->setEditable(false);
  connect(hostAPIList, SIGNAL(currentIndexChanged(int)),
          this, SLOT(hostAPIIndexChanged(int)));

  QVBoxLayout *vlayout = new QVBoxLayout;
  QFormLayout *formLayout = new QFormLayout;
  formLayout->setSpacing(5);
  formLayout->setContentsMargins(2, 2, 2, 2);
  formLayout->addRow(tr("&Input device:"), inputDeviceList);
  formLayout->addRow(new QLabel, unmuteLocalChannelsBox);
  formLayout->addRow(tr("&Output device:"), outputDeviceList);
  formLayout->addRow(new QLabel); /* just a spacer */
  formLayout->addRow(tr("Sample &rate (Hz):"), sampleRateList);
  formLayout->addRow(tr("&Latency (ms):"), latencyList);
  formLayout->addRow(invalidSettingsWidget);
  formLayout->addRow(validSettingsWidget);
  formLayout->addRow(new QLabel); /* just a spacer */
  formLayout->addRow(new QLabel(tr("<b>Troubleshooting:</b> If you experience audio problems, try selecting another audio system.")));
  formLayout->addRow(tr("Audio &system:"), hostAPIList);
  vlayout->addLayout(formLayout);
  setLayout(vlayout);

  populateHostAPIList();
}

void PortAudioSettingsPage::populateHostAPIList()
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

void PortAudioSettingsPage::willValidateSettings()
{
  validateSettingsEntryCount++;
}

void PortAudioSettingsPage::validateSettings()
{
  /* Avoid repeating validation because Pa_IsFormatSupported() can be
   * slow and we should not block the GUI thread for too long.
   */
  if (--validateSettingsEntryCount > 1) {
    return;
  }

  double sampleRate = sampleRateList->currentText().toDouble();
  double latency = latencyList->currentText().toDouble() / 1000;

  if (sampleRate == 0 ||
      latency == 0 ||
      inputDeviceList->currentIndex() < 0 ||
      outputDeviceList->currentIndex() < 0) {
    invalidSettingsWidget->setVisible(true);
    validSettingsWidget->setVisible(false);
    return;
  }

  PaStreamParameters inputParams;
  inputParams.device = inputDeviceList->itemData(inputDeviceList->currentIndex()).toInt(NULL);
  inputParams.channelCount = 1 /* TODO mono */;
  inputParams.sampleFormat = paFloat32 | paNonInterleaved;
  inputParams.suggestedLatency = latency;
  inputParams.hostApiSpecificStreamInfo = NULL;

  PaStreamParameters outputParams = inputParams;
  outputParams.device = outputDeviceList->itemData(outputDeviceList->currentIndex()).toInt(NULL);
  outputParams.channelCount = 1 /* TODO mono */;

  PaError error = Pa_IsFormatSupported(&inputParams, &outputParams, sampleRate);
  invalidSettingsWidget->setVisible(error != paFormatIsSupported);
  validSettingsWidget->setVisible(error == paFormatIsSupported);
}

/* Return PaDeviceInfo* or NULL if not found */
static const PaDeviceInfo *lookupDeviceInfo(QComboBox *deviceList)
{
  int index = deviceList->currentIndex();
  if (index < 0) {
    return NULL;
  }
  PaDeviceIndex deviceIndex = deviceList->itemData(index).toInt(NULL);
  return Pa_GetDeviceInfo(deviceIndex);
}

void PortAudioSettingsPage::autoselectSampleRate()
{
  const PaDeviceInfo *inputDeviceInfo = lookupDeviceInfo(inputDeviceList);
  if (!inputDeviceInfo) {
    return;
  }

  const PaDeviceInfo *outputDeviceInfo = lookupDeviceInfo(outputDeviceList);
  if (!outputDeviceInfo) {
    return;
  }

  double sampleRate = qMin(inputDeviceInfo->defaultSampleRate,
                           outputDeviceInfo->defaultSampleRate);
  int index = sampleRateList->findText(QString::number(sampleRate));
  if (index != -1) {
    sampleRateList->setCurrentIndex(index);
  }
}

void PortAudioSettingsPage::autoselectLatency()
{
  const PaDeviceInfo *inputDeviceInfo = lookupDeviceInfo(inputDeviceList);
  if (!inputDeviceInfo) {
    return;
  }

  const PaDeviceInfo *outputDeviceInfo = lookupDeviceInfo(outputDeviceList);
  if (!outputDeviceInfo) {
    return;
  }

  double latency = qMax(inputDeviceInfo->defaultLowInputLatency,
                        outputDeviceInfo->defaultLowOutputLatency) * 1000;
  int i;
  for (i = 0; i < latencyList->count(); i++) {
    if (latency < latencyList->itemText(i).toDouble() - .01 /* epsilon */) {
      if (i > 0) {
        i--;
      }
      latencyList->setCurrentIndex(i);
      return;
    }
  }
}

void PortAudioSettingsPage::setupLatencyList()
{
  latencyList->clear();

  /* Enumerate latencies for power-of-2 buffers up to 4096 frames.  Start
   * at 1 millisecond since lower latencies are unlikely to produce
   * glitch-free audio.
   */
  double sampleRate = sampleRateList->currentText().toDouble();
  if (sampleRate <= 0) {
    return;
  }
  int framesPerMillisecond = 1 << (int)ceil(log2(sampleRate / 1000));
  for (int i = framesPerMillisecond; i < 4096; i *= 2) {
    latencyList->addItem(QString::number(i / sampleRate * 1000, 'g', 3));
  }
}

void PortAudioSettingsPage::deviceIndexChanged(int index)
{
  willValidateSettings();
  autoselectSampleRate();
  validateSettings();
}

void PortAudioSettingsPage::sampleRateIndexChanged(int index)
{
  willValidateSettings();
  setupLatencyList();
  autoselectLatency();
  validateSettings();
}

void PortAudioSettingsPage::latencyIndexChanged(int index)
{
  willValidateSettings();
  validateSettings();
}

void PortAudioSettingsPage::hostAPIIndexChanged(int index)
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

QString PortAudioSettingsPage::hostAPI() const
{
  return hostAPIList->currentText();
}

void PortAudioSettingsPage::setHostAPI(const QString &name)
{
  int i = hostAPIList->findText(name);
  if (i != -1) {
    hostAPIList->setCurrentIndex(i);
    return;
  }

  /* Pick default host API based on this list of priorities */
  const int hostAPIPriority[] = {
    0, /* paInDevelopment */
    1, /* paDirectSound */
    0, /* paMME */
    4, /* paASIO */
    0, /* paSoundManager */
    1, /* paCoreAudio */
    0, /* <empty> */
    0, /* paOSS */
    1, /* paALSA */
    0, /* paAL */
    0, /* paBeOS */
    3, /* paWDMKS */
    2, /* paJACK */
    2, /* paWASAPI */
    0, /* paAudioScienceHPI */
  };
  const PaHostApiTypeId numTypes =
    (PaHostApiTypeId)(sizeof(hostAPIPriority) / sizeof(hostAPIPriority[0]));
  int pri = -1;

  for (int j = 0; j < hostAPIList->count(); j++) {
    PaHostApiIndex apiIndex = hostAPIList->itemData(j).toInt(NULL);
    const PaHostApiInfo *hostAPIInfo = Pa_GetHostApiInfo(apiIndex);
    if (!hostAPIInfo || hostAPIInfo->type >= numTypes) {
      continue;
    }
    if (hostAPIPriority[hostAPIInfo->type] > pri) {
      pri = hostAPIPriority[hostAPIInfo->type];
      i = j;
    }
  }
  if (i != -1) {
    hostAPIList->setCurrentIndex(i);
  }
}

QString PortAudioSettingsPage::inputDevice() const
{
  return inputDeviceList->currentText();
}

void PortAudioSettingsPage::setInputDevice(const QString &name)
{
  int i = inputDeviceList->findText(name);
  if (i >= 0) {
    inputDeviceList->setCurrentIndex(i);
  }
}

bool PortAudioSettingsPage::unmuteLocalChannels() const
{
  return unmuteLocalChannelsBox->isChecked();
}

void PortAudioSettingsPage::setUnmuteLocalChannels(bool unmute)
{
  unmuteLocalChannelsBox->setChecked(unmute);
}

QString PortAudioSettingsPage::outputDevice() const
{
  return outputDeviceList->currentText();
}

void PortAudioSettingsPage::setOutputDevice(const QString &name)
{
  int i = outputDeviceList->findText(name);
  if (i >= 0) {
    outputDeviceList->setCurrentIndex(i);
  }
}

double PortAudioSettingsPage::sampleRate() const
{
  return sampleRateList->currentText().toDouble();
}

void PortAudioSettingsPage::setSampleRate(double sampleRate)
{
  int i = sampleRateList->findText(QString::number(sampleRate));
  if (i >= 0) {
    sampleRateList->setCurrentIndex(i);
  }
}

double PortAudioSettingsPage::latency() const
{
  return latencyList->currentText().toDouble() / 1000;
}

void PortAudioSettingsPage::setLatency(double latency)
{
  int i = latencyList->findText(QString::number(latency * 1000, 'g', 3));
  if (i >= 0) {
    latencyList->setCurrentIndex(i);
  }
}
