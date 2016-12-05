/*
    Copyright (C) 2016 Stefan Hajnoczi <stefanha@gmail.com>

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

#include <QFormLayout>
#include <QLabel>
#include "UISettingsPage.h"

UISettingsPage::UISettingsPage(QWidget *parent)
  : QWidget(parent)
{
  chatFontSizeList = new QComboBox;
  chatFontSizeList->setEditable(false);
  chatFontSizeList->addItem("Default");
  chatFontSizeList->addItem("10");
  chatFontSizeList->addItem("12");
  chatFontSizeList->addItem("14");
  chatFontSizeList->addItem("16");
  chatFontSizeList->addItem("18");
  chatFontSizeList->addItem("20");
  chatFontSizeList->addItem("22");
  chatFontSizeList->addItem("24");
  chatFontSizeList->addItem("26");
  chatFontSizeList->addItem("28");
  chatFontSizeList->addItem("32");
  chatFontSizeList->addItem("36");
  chatFontSizeList->addItem("40");
  chatFontSizeList->addItem("44");
  chatFontSizeList->addItem("48");
  chatFontSizeList->addItem("54");
  chatFontSizeList->addItem("60");

  QFormLayout *formLayout = new QFormLayout;
  formLayout->setSpacing(5);
  formLayout->setContentsMargins(2, 2, 2, 2);
  formLayout->addRow(tr("Chat &font size:"), chatFontSizeList);
  setLayout(formLayout);
}

int UISettingsPage::chatFontSize() const
{
  return chatFontSizeList->currentText().toInt();
}

void UISettingsPage::setChatFontSize(int size)
{
  int i = chatFontSizeList->findText(QString::number(size));
  if (i < 0) {
    i = 0;
  }
  chatFontSizeList->setCurrentIndex(i);
}
