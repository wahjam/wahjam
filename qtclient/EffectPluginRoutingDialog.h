/*
    Copyright (C) 2014 Stefan Hajnoczi <stefanha@gmail.com>

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

#ifndef _EFFECTPLUGINROUTINGDIALOG_H_
#define _EFFECTPLUGINROUTINGDIALOG_H_

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include "PluginScanner.h"

class EffectPluginRoutingDialog : public QDialog
{
  Q_OBJECT
  Q_PROPERTY(float wetDryMix READ getWetDryMix WRITE setWetDryMix)
  Q_PROPERTY(bool receiveMidi READ getReceiveMidi WRITE setReceiveMidi)

public:
  EffectPluginRoutingDialog(const QString &name, QWidget *parent = 0);
  ~EffectPluginRoutingDialog();

  float getWetDryMix() const;
  void setWetDryMix(float wetDryMix);
  bool getReceiveMidi() const;
  void setReceiveMidi(bool receive);

private slots:

private:
  QComboBox *wetDryMixList;
  QCheckBox *receiveMidiCheckbox;
};

#endif /* _EFFECTPLUGINROUTINGDIALOG_H_ */
