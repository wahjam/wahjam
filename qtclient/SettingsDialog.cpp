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

#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include "SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
  : QDialog(parent)
{
  pageList = new QListWidget;
  pageList->setMaximumWidth(192);
  connect(pageList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
          this, SLOT(changePage(QListWidgetItem*, QListWidgetItem*)));

  pageStack = new QStackedWidget;

  QDialogButtonBox *dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(dialogButtonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()),
          this, SLOT(close()));

  QHBoxLayout *hboxLayout = new QHBoxLayout;
  hboxLayout->addWidget(pageList);
  hboxLayout->addWidget(pageStack, 1);

  QVBoxLayout *vboxLayout = new QVBoxLayout;
  vboxLayout->addLayout(hboxLayout);
  vboxLayout->addStretch(1);
  vboxLayout->addSpacing(12);
  vboxLayout->addWidget(dialogButtonBox);
  setLayout(vboxLayout);

  setWindowTitle(tr("Settings"));
}

void SettingsDialog::addPage(const QString &label, QWidget *page)
{
  pageList->addItem(label);
  pageStack->addWidget(page);

  if (pageList->count() == 1) {
    pageList->setCurrentRow(0);
  }
}

void SettingsDialog::setPage(int index)
{
  pageList->setCurrentRow(index);
}

void SettingsDialog::changePage(QListWidgetItem *current,
                                QListWidgetItem *previous)
{
  if (!current) {
    current = previous;
  }

  pageStack->setCurrentIndex(pageList->row(current));
}
