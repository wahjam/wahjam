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
  : QWidget(parent)
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

  sampleRateList = new QComboBox;
  sampleRateList->setEditable(false);
  connect(sampleRateList, SIGNAL(currentIndexChanged(int)),
          this, SLOT(sampleRateIndexChanged(int)));

  latencyList = new QComboBox();

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
  formLayout->addRow(new QLabel); /* just a spacer */
  formLayout->addRow(new QLabel(tr("<b>Troubleshooting:</b> If you experience audio problems, try selecting another audio system.")));
  formLayout->addRow(tr("Audio &system:"), hostAPIList);
  vlayout->addLayout(formLayout);
  setLayout(vlayout);

  populateHostAPIList();
  autoselectHostAPI();
}

void PortAudioSettingsPage::populateHostAPIList()
{
  PaHostApiIndex i;
  hostAPIList->clear();
  for (i = 0; i < Pa_GetHostApiCount(); i++) {
    const PaHostApiInfo *hostAPIInfo = Pa_GetHostApiInfo(i);
    if (!hostAPIInfo) {
      continue;
    }
    if (hostAPIInfo->deviceCount <= 0) {
      continue;
    }
    QString name = QString::fromUtf8(hostAPIInfo->name);
    hostAPIList->addItem(name, i);
  }
}

void PortAudioSettingsPage::autoselectHostAPI()
{
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
  int i = -1;

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

void PortAudioSettingsPage::hostAPIIndexChanged(int)
{
  populateDeviceList();
  autoselectDevice();
}

void PortAudioSettingsPage::populateDeviceList()
{
  int index = hostAPIList->currentIndex();

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
    QString name = QString::fromUtf8(devInfo->name);

    if (devInfo->maxInputChannels > 0) {
      inputDeviceList->addItem(name, devIndex);
    }
    if (devInfo->maxOutputChannels > 0) {
      outputDeviceList->addItem(name, devIndex);
    }
  }
}

void PortAudioSettingsPage::autoselectDevice()
{
  int index = hostAPIList->currentIndex();
  if (index < 0) {
    return;
  }

  PaHostApiIndex apiIndex = hostAPIList->itemData(index).toInt(NULL);
  const PaHostApiInfo *hostAPIInfo = Pa_GetHostApiInfo(apiIndex);
  if (!hostAPIInfo) {
    return;
  }

  int i;
  for (i = 0; i < inputDeviceList->count(); i++) {
    if (hostAPIInfo->defaultInputDevice ==
        inputDeviceList->itemData(i).toInt(NULL)) {
      inputDeviceList->setCurrentIndex(i);
      break;
    }
  }
  for (i = 0; i < outputDeviceList->count(); i++) {
    if (hostAPIInfo->defaultOutputDevice ==
        outputDeviceList->itemData(i).toInt(NULL)) {
      outputDeviceList->setCurrentIndex(i);
      break;
    }
  }
}

void PortAudioSettingsPage::deviceIndexChanged(int)
{
  populateSampleRateList();
  autoselectSampleRate();
}

void PortAudioSettingsPage::populateSampleRateList()
{
  const double rates[] = {32000, 44100, 48000, 88200, 96000};
  size_t i;

  PaStreamParameters inputParams;
  inputParams.device = inputDeviceList->itemData(inputDeviceList->currentIndex()).toInt(NULL);
  inputParams.sampleFormat = paFloat32 | paNonInterleaved;
  inputParams.suggestedLatency = 0;
  inputParams.hostApiSpecificStreamInfo = NULL;

  const PaDeviceInfo *inputDeviceInfo = Pa_GetDeviceInfo(inputParams.device);
  if (inputDeviceInfo && inputDeviceInfo->maxInputChannels > 1) {
    inputParams.channelCount = 2;
  } else {
    inputParams.channelCount = 1;
  }

  PaStreamParameters outputParams = inputParams;
  outputParams.device = outputDeviceList->itemData(outputDeviceList->currentIndex()).toInt(NULL);

  const PaDeviceInfo *outputDeviceInfo = Pa_GetDeviceInfo(outputParams.device);
  if (outputDeviceInfo) {
    outputParams.channelCount = outputDeviceInfo->maxOutputChannels;
  } else {
    outputParams.channelCount = 1;
  }

  sampleRateList->clear();
  for (i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
    if (Pa_IsFormatSupported(&inputParams,
                             &outputParams,
                             rates[i]) == paFormatIsSupported) {
      sampleRateList->addItem(QString::number(rates[i]));
    }
  }
}

void PortAudioSettingsPage::autoselectSampleRate()
{
  const double preferredRates[] = {44100, 48000, 88200, 96000, 32000};
  size_t i;
  for (i = 0; i < sizeof(preferredRates) / sizeof(preferredRates[0]); i++) {
    int index = sampleRateList->findText(QString::number(preferredRates[i]));
    if (index != -1) {
      sampleRateList->setCurrentIndex(index);
      return;
    }
  }
}

void PortAudioSettingsPage::sampleRateIndexChanged(int)
{
  populateLatencyList();
  autoselectLatency();
}

void PortAudioSettingsPage::populateLatencyList()
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

  /* Default to 10 milliseconds or higher to prevent drop-outs */
  double latency = qMax(qMax(inputDeviceInfo->defaultLowInputLatency,
                             outputDeviceInfo->defaultLowOutputLatency),
                        0.01) * 1000;
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

  autoselectHostAPI();
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
