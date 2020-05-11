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

#include <QVBoxLayout>
#include "LockableSettingsPage.h"

LockableSettingsPage::LockableSettingsPage(const QString &unlockButtonText, QWidget *parent)
  : QWidget(parent)
{
  unlockButton = new QPushButton(unlockButtonText);
  connect(unlockButton, SIGNAL(clicked()),
          this, SIGNAL(unlockRequest()));

  contentsWidget = new QWidget;

  QVBoxLayout *vlayout = new QVBoxLayout;
  vlayout->addWidget(unlockButton);
  vlayout->addWidget(contentsWidget);
  setLayout(vlayout);

  setLocked(false);
}

void LockableSettingsPage::setLocked(bool locked)
{
  unlockButton->setVisible(locked);
  contentsWidget->setEnabled(!locked);
}
