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

#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLibrary>
#include <QApplication>
#include "../VSTPlugin.h"
#include "AddVSTPluginDialog.h"

AddVSTPluginDialog::AddVSTPluginDialog(QWidget *parent)
  : QDialog(parent)
{
  QVBoxLayout *vBoxLayout = new QVBoxLayout;

  QLabel *searchPathLabel = new QLabel(tr("Search path:"));
  vBoxLayout->addWidget(searchPathLabel);

  QHBoxLayout *hBoxLayout = new QHBoxLayout;
  searchPathEdit = new QLineEdit;
  searchPathEdit->setMinimumWidth(400);
  hBoxLayout->addWidget(searchPathEdit, 4);
  QPushButton *addSearchPathButton = new QPushButton(QIcon::fromTheme("list-add"), tr("Add"));
  connect(addSearchPathButton, SIGNAL(clicked()),
          this, SLOT(addSearchPath()));
  hBoxLayout->addWidget(addSearchPathButton);
  QPushButton *scanButton = new QPushButton(QIcon::fromTheme("view-refresh"), tr("Scan"));
  connect(scanButton, SIGNAL(clicked()),
          this, SLOT(scan()));
  hBoxLayout->addWidget(scanButton);
  vBoxLayout->addLayout(hBoxLayout);

  vBoxLayout->addWidget(new QLabel(tr("<b>Note:</b> If you encounter a crash while scanning, narrow down the search path or move known good plugins elsewhere.")));
  vBoxLayout->addSpacing(12);

  QLabel *pluginsLabel = new QLabel(tr("Available plugins:"));
  vBoxLayout->addWidget(pluginsLabel);
  pluginsList = new QListWidget;
  connect(pluginsList, SIGNAL(itemSelectionChanged()),
          this, SLOT(itemSelectionChanged()));
  vBoxLayout->addWidget(pluginsList);

  QDialogButtonBox *dialogButtonBox;
  dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                         QDialogButtonBox::Cancel);
  okButton = dialogButtonBox->button(QDialogButtonBox::Ok);
  connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
  connect(dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
  vBoxLayout->addWidget(dialogButtonBox);

  setLayout(vBoxLayout);
  setWindowTitle(tr("Add VST Plugin..."));
  itemSelectionChanged();
}

QString AddVSTPluginDialog::searchPath() const
{
  return searchPathEdit->text();
}

void AddVSTPluginDialog::setSearchPath(const QString &path)
{
  searchPathEdit->setText(path);
}

QString AddVSTPluginDialog::fileName() const
{
  QListWidgetItem *item = pluginsList->currentItem();
  if (!item) {
    return "";
  }
  return item->data(Qt::ToolTipRole).toString();
}

void AddVSTPluginDialog::addSearchPath()
{
  QFileDialog dialog(this, tr("Add to search path..."));
  dialog.setFileMode(QFileDialog::Directory);
  dialog.setOption(QFileDialog::ShowDirsOnly, true);
  if (dialog.exec()) {
    QString searchPath = searchPathEdit->text();
    if (!searchPath.isEmpty()) {
      searchPath += ";";
    }
    searchPath += dialog.selectedFiles().join(";");
    searchPathEdit->setText(searchPath);
  }
}

void AddVSTPluginDialog::scan()
{
  QStringList searchPaths = searchPathEdit->text().split(";");

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  pluginsList->clear();
  while (!searchPaths.isEmpty()) {
    QDir dir(searchPaths.takeFirst());

    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::Readable |
                                        QDir::Executable |
                                        QDir::NoDotAndDotDot);
    QString subdir;
    foreach (subdir, subdirs) {
      searchPaths.append(dir.filePath(subdir));
    }

    QStringList files = dir.entryList(QDir::Files);
    QString file;
    foreach (file, files) {
      if (!QLibrary::isLibrary(file)) {
        continue;
      }

      file = dir.filePath(file);
      VSTPlugin vst(file);
      if (!vst.load()) {
        continue;
      }

      QListWidgetItem *item = new QListWidgetItem(vst.getName());
      item->setData(Qt::ToolTipRole, file);
      pluginsList->addItem(item);

      /* Process UI thread events, scanning plugins might take a while */
      QCoreApplication::processEvents();
    }
  }

  QApplication::restoreOverrideCursor();
}

void AddVSTPluginDialog::itemSelectionChanged()
{
  okButton->setEnabled(pluginsList->currentItem());
}
