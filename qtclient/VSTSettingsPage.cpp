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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include "qtclient.h"
#include "VSTSettingsPage.h"

VSTSettingsPage::VSTSettingsPage(VSTProcessor *processor_, QWidget *parent)
  : QWidget(parent), processor(processor_), addPluginDialog(this)
{
  int i;

  addPluginDialog.setSearchPath(settings->value("vst/searchPath").toString());
  addPluginDialog.setPlugins(settings->value("vst/plugins").toStringList());

  QVBoxLayout *vBoxLayout = new QVBoxLayout;
  QHBoxLayout *hBoxLayout = new QHBoxLayout;

  pluginList = new QListWidget;
  connect(pluginList, SIGNAL(itemSelectionChanged()),
          this, SLOT(itemSelectionChanged()));
  for (i = 0; i < processor->numPlugins(); i++) {
    pluginList->addItem(processor->getPlugin(i)->getName());
  }
  hBoxLayout->addWidget(pluginList);

  QVBoxLayout *buttonLayout = new QVBoxLayout;
  QPushButton *addButton = new QPushButton(QIcon::fromTheme("list-add"), tr("Add"));
  connect(addButton, SIGNAL(clicked()),
          this, SLOT(addPlugin()));
  buttonLayout->addWidget(addButton);
  removeButton = new QPushButton(QIcon::fromTheme("list-remove"), tr("Remove"));
  connect(removeButton, SIGNAL(clicked()),
          this, SLOT(removePlugin()));
  buttonLayout->addWidget(removeButton);
  upButton = new QPushButton(QIcon::fromTheme("go-up"), tr("Up"));
  connect(upButton, SIGNAL(clicked()),
          this, SLOT(movePluginUp()));
  buttonLayout->addWidget(upButton);
  downButton = new QPushButton(QIcon::fromTheme("go-down"), tr("Down"));
  connect(downButton, SIGNAL(clicked()),
          this, SLOT(movePluginDown()));
  buttonLayout->addWidget(downButton);
  editButton = new QPushButton(tr("Edit..."));
  connect(editButton, SIGNAL(clicked()),
          this, SLOT(openEditor()));
  buttonLayout->addWidget(editButton);
  hBoxLayout->addLayout(buttonLayout);

  vBoxLayout->addLayout(hBoxLayout);

  setLayout(vBoxLayout);
  itemSelectionChanged();
}

void VSTSettingsPage::itemSelectionChanged()
{
  bool selected = pluginList->currentItem();

  removeButton->setEnabled(selected);
  upButton->setEnabled(selected);
  downButton->setEnabled(selected);
  editButton->setEnabled(selected);
}

void VSTSettingsPage::addPlugin()
{
  if (!addPluginDialog.exec()) {
    return;
  }
  settings->setValue("vst/searchPath", addPluginDialog.searchPath());
  settings->setValue("vst/plugins", addPluginDialog.plugins());

  VSTPlugin *vst = new VSTPlugin(addPluginDialog.fileName());
  if (!vst->load()) {
    delete vst;
    return;
  }

  /* Opening the editor window frequently blocks for a short amount of time.
   * Do it before inserting the plugin so audio processing is unaffected.
   */
  vst->openEditor(parentWidget());

  if (!processor->insertPlugin(0, vst)) {
    delete vst;
    return;
  }

  pluginList->insertItem(0, vst->getName());
  pluginList->setCurrentRow(0);
}

void VSTSettingsPage::removePlugin()
{
  int index = pluginList->currentRow();
  QListWidgetItem *item = pluginList->takeItem(index);

  processor->removePlugin(index);
  pluginList->removeItemWidget(item);
  delete item;
}

void VSTSettingsPage::movePluginUp()
{
  int index = pluginList->currentRow();
  if (index == 0) {
    return;
  }

  QListWidgetItem *item = pluginList->takeItem(index);
  pluginList->insertItem(index - 1, item);
  pluginList->setCurrentRow(index - 1);

  processor->moveUp(index);
}

void VSTSettingsPage::movePluginDown()
{
  int index = pluginList->currentRow();
  if (index + 1 == pluginList->count()) {
    return;
  }

  QListWidgetItem *item = pluginList->takeItem(index);
  pluginList->insertItem(index + 1, item);
  pluginList->setCurrentRow(index + 1);

  processor->moveDown(index);
}

void VSTSettingsPage::openEditor()
{
  int index = pluginList->currentRow();
  VSTPlugin *vst = processor->getPlugin(index);

  vst->openEditor(parentWidget());
}
