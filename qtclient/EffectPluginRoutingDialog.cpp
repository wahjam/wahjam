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

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include "EffectPluginRoutingDialog.h"

EffectPluginRoutingDialog::EffectPluginRoutingDialog(const QString &name, QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Route %1...").arg(name));

  QVBoxLayout *layout = new QVBoxLayout;

  QWidget *form = new QWidget;
  QFormLayout *formLayout = new QFormLayout;

  wetDryMixList = new QComboBox;
  wetDryMixList->addItem("On", 1.0f);
  wetDryMixList->addItem("Mix", 0.5f);
  wetDryMixList->addItem("Bypass", 0.0f);
  formLayout->addRow(tr("&Audio:"), wetDryMixList);

  receiveMidiCheckbox = new QCheckBox;
  formLayout->addRow(tr("&MIDI in:"), receiveMidiCheckbox);

  form->setLayout(formLayout);
  layout->addWidget(form);

  QDialogButtonBox *dialogButtonBox;
  dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Apply |
                                         QDialogButtonBox::Cancel);
  QPushButton *applyButton = dialogButtonBox->button(QDialogButtonBox::Apply);
  connect(applyButton, SIGNAL(clicked()), this, SLOT(accept()));
  connect(dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

  layout->addWidget(dialogButtonBox);
  setLayout(layout);
}

EffectPluginRoutingDialog::~EffectPluginRoutingDialog()
{
}

float EffectPluginRoutingDialog::getWetDryMix() const
{
  return wetDryMixList->itemData(wetDryMixList->currentIndex()).toFloat();
}

void EffectPluginRoutingDialog::setWetDryMix(float wetDryMix)
{
  if (wetDryMix < 0.49) {
    wetDryMixList->setCurrentIndex(2);
  } else if (wetDryMix > 0.51) {
    wetDryMixList->setCurrentIndex(0);
  } else {
    wetDryMixList->setCurrentIndex(1);
  }
}

bool EffectPluginRoutingDialog::getReceiveMidi() const
{
  return receiveMidiCheckbox->isChecked();
}

void EffectPluginRoutingDialog::setReceiveMidi(bool receive)
{
  receiveMidiCheckbox->setChecked(receive);
}
