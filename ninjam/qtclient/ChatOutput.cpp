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

#include "ChatOutput.h"

#ifndef AUTOLINK_REGEXP
#define AUTOLINK_REGEXP "https?://[-_.!~*'()a-zA-Z0-9;/?:@&=+$,%#]+"
#endif


ChatOutput::ChatOutput(QWidget *parent)
  : QTextBrowser(parent), autolinkRegexp(AUTOLINK_REGEXP)
{
  setReadOnly(true);
  setOpenLinks(false);
  setUndoRedoEnabled(false);

  normalFormat.setFontWeight(QFont::Normal);

  boldFormat.setFontWeight(QFont::Bold);

  linkFormat.setAnchor(true);
  linkFormat.setAnchorHref("");
  linkFormat.setFontWeight(QFont::Bold);
  linkFormat.setForeground(palette().link());
  linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

  cursor = textCursor();

  Q_ASSERT(autolinkRegexp.isValid());
}

void ChatOutput::addText(const QString &text, const QTextCharFormat &format)
{
  cursor.insertText(text, format);
}

/**
 * add content with autolink
 */
void ChatOutput::addContent(const QString &content, const QTextCharFormat &format)
{
  QString text;
  QString url;
  int offset = 0;
  int index = autolinkRegexp.indexIn(content, offset);

  // avoid QString::mid call, content does not have links most cases.
  if (index == -1) {
    Q_ASSERT(offset == 0);
    addText(content, format);
    return;
  }

  do {
    Q_ASSERT(index == -1 || index >= 0);

    // add normal text.
    // when index was -1, no more links, that add the rest of content.
    text = content.mid(offset, (index == -1) ? -1 : (index-offset));
    if (!text.isEmpty()) {
      addText(text, format);
    }

    if (index == -1) {
      break;
    }

    url = autolinkRegexp.cap(0);
    if (!url.isEmpty()) {
      addLink(url, url);
    }

    // set variables for next iteration.
    offset = index + autolinkRegexp.matchedLength();
    index = autolinkRegexp.indexIn(content, offset);

    // NOTE: overflow check ((int + int) >= 0)
    Q_ASSERT(offset >= 0);

  } while (offset >= 0);
}

void ChatOutput::addLine(const QString &prefix, const QString &content)
{
  // Edit block for Undo/Redo.
  cursor.beginEditBlock();
  {
    cursor.movePosition(QTextCursor::End);

    if (!document()->isEmpty()) {
      cursor.insertBlock();
    }

    if (!prefix.isEmpty()) {
      addText(prefix, boldFormat);
    }

    if (!content.isEmpty()) {
      addContent(content, normalFormat);
    }
  }
  cursor.endEditBlock();

  // Autoscroll bottom of chat
  moveCursor(QTextCursor::End);
}

void ChatOutput::addLink(const QString &href, const QString &linktext)
{
  QTextCharFormat format = linkFormat;
  format.setAnchorHref(href);

  addText(linktext, format);
}

void ChatOutput::addMessage(const QString &src, const QString &message)
{
  addLine(tr("<%1> ").arg(src), message);
}

void ChatOutput::addActionMessage(const QString &src, const QString &message)
{
  addLine(tr("* %1 ").arg(src), message);
}

void ChatOutput::addPrivateMessage(const QString &src, const QString &message)
{
  addLine(tr("* %1 ").arg(src), message);
}

void ChatOutput::addInfoMessage(const QString &message)
{
  addLine(tr("[INFO] "), message);
}

void ChatOutput::addErrorMessage(const QString &message)
{
  addLine(tr("[ERROR] "), message);
}

