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

#ifndef _VSTCONFIGDIALOG_H_
#define _VSTCONFIGDIALOG_H_

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include "VSTProcessor.h"
#include "AddVSTPluginDialog.h"

class VSTConfigDialog : public QDialog
{
  Q_OBJECT

public:
  VSTConfigDialog(VSTProcessor *processor, QWidget *parent = NULL);

private slots:
  void itemSelectionChanged();
  void addPlugin();
  void removePlugin();
  void movePluginUp();
  void movePluginDown();
  void openEditor();

private:
  VSTProcessor *processor;
  AddVSTPluginDialog addPluginDialog;
  QListWidget *pluginList;
  QPushButton *removeButton;
  QPushButton *upButton;
  QPushButton *downButton;
  QPushButton *editButton;
};

#endif /* _VSTCONFIGDIALOG_H_ */
