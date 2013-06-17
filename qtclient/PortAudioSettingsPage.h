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

#ifndef _PORTAUDIOSETTINGSPAGE_H_
#define _PORTAUDIOSETTINGSPAGE_H_

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>

class PortAudioSettingsPage : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString hostAPI READ hostAPI WRITE setHostAPI)
  Q_PROPERTY(QString inputDevice READ inputDevice WRITE setInputDevice)
  Q_PROPERTY(bool unmuteLocalChannels READ unmuteLocalChannels WRITE setUnmuteLocalChannels)
  Q_PROPERTY(QString outputDevice READ outputDevice WRITE setOutputDevice)
  Q_PROPERTY(double sampleRate READ sampleRate WRITE setSampleRate)
  Q_PROPERTY(double latency READ latency WRITE setLatency)

public:
  PortAudioSettingsPage(QWidget *parent = 0);
  void populateHostAPIList();
  QString hostAPI() const;
  void setHostAPI(const QString &name);
  QString inputDevice() const;
  void setInputDevice(const QString &name);
  bool unmuteLocalChannels() const;
  void setUnmuteLocalChannels(bool unmute);
  QString outputDevice() const;
  void setOutputDevice(const QString &name);
  double sampleRate() const;
  void setSampleRate(double sampleRate);
  double latency() const;
  void setLatency(double latency);

private slots:
  void deviceIndexChanged(int index);
  void sampleRateIndexChanged(int index);
  void latencyIndexChanged(int index);
  void hostAPIIndexChanged(int index);

private:
  QComboBox *hostAPIList;
  QComboBox *inputDeviceList;
  QCheckBox *unmuteLocalChannelsBox;
  QComboBox *outputDeviceList;
  QComboBox *sampleRateList;
  QComboBox *latencyList;
  QWidget *invalidSettingsWidget;
  QWidget *validSettingsWidget;
  int validateSettingsEntryCount;

  void willValidateSettings();
  void validateSettings();
  void autoselectSampleRate();
  void autoselectLatency();
  void setupLatencyList();
};

#endif /* _PORTAUDIOSETTINGSPAGE_H_ */
