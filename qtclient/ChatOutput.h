/*
    Copyright (C) 2012 Ikkei Shimomura (tea) <Ikkei.Shimomura@gmail.com>

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

#ifndef _CHATOUTPUT_H_
#define _CHATOUTPUT_H_

#include <QRegExp>
#include <QTextBrowser>


class ChatOutput : public QTextBrowser
{
  Q_OBJECT

public:
  ChatOutput(QWidget *parent=0);

  void setFontSize(int size);

public slots:
  void addText(const QString &text, const QTextCharFormat &format);
  void addContent(const QString &content, const QTextCharFormat &format);
  void addLine(const QString &prefix, const QString &content);
  void addLink(const QString &href, const QString &linktext);

  void addMessage(const QString &src, const QString &message);
  void addActionMessage(const QString &src, const QString &message);
  void addPrivateMessage(const QString &src, const QString &message);
  void addInfoMessage(const QString &message);
  void addErrorMessage(const QString &message);
  void addBannerMessage(const QString &message);

private:
  QRegExp autolinkRegexp;
  QTextCharFormat normalFormat;
  QTextCharFormat boldFormat;
  QTextCharFormat linkFormat;
  QTextCursor cursor;

};

#endif /* _CHATOUTPUT_H_ */

