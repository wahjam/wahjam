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

#ifndef _UISETTINGSPAGE_H_
#define _UISETTINGSPAGE_H_

#include <QWidget>
#include <QComboBox>

class UISettingsPage : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(int chatFontSize READ chatFontSize WRITE setChatFontSize)

public:
  UISettingsPage(QWidget *parent = 0);

  int chatFontSize() const;
  void setChatFontSize(int size);

private:
  QComboBox *chatFontSizeList;
};

#endif /* _UISETTINGSPAGE_H_ */
