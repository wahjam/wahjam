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
#include <QFormLayout>
#include <QPushButton>

#include "ConnectDialog.h"

ConnectDialog::ConnectDialog(QWidget *parent)
  : QDialog(parent)
{
  serverBrowser = new ServerBrowser(this);
  connect(serverBrowser, SIGNAL(serverItemClicked(const QString &)),
          this, SLOT(setHost(const QString &)));
  connect(serverBrowser, SIGNAL(serverItemActivated(const QString &)),
          this, SLOT(onServerSelected(const QString &)));

  hostEdit = new QComboBox;
  hostEdit->setEditable(true);

  userEdit = new QLineEdit;
  passEdit = new QLineEdit;
  passEdit->setEchoMode(QLineEdit::Password);
  passEdit->setEnabled(false);

  QPushButton *connectButton = new QPushButton(tr("&Connect"));
  connect(connectButton, SIGNAL(clicked()), this, SLOT(accept()));

  QVBoxLayout *layout = new QVBoxLayout;
  QWidget *form = new QWidget;
  QFormLayout *formLayout = new QFormLayout;
  formLayout->addRow(tr("&Server:"), hostEdit);
  formLayout->addRow(tr("&Username:"), userEdit);

  publicCheckbox = new QCheckBox(tr("&Public server"));
  publicCheckbox->setChecked(true);
  connect(publicCheckbox, SIGNAL(stateChanged(int)), this, SLOT(publicServerStateChanged(int)));
  formLayout->addWidget(publicCheckbox);

  formLayout->addRow(tr("&Password:"), passEdit);
  form->setLayout(formLayout);
  layout->addWidget(serverBrowser);
  layout->addWidget(form);
  layout->addWidget(connectButton);
  setLayout(layout);
  setWindowTitle(tr("Connect to server..."));
}

void ConnectDialog::publicServerStateChanged(int state)
{
  passEdit->setEnabled(state == Qt::Unchecked);
}

void ConnectDialog::setRecentHostsList(const QStringList &hosts)
{
  hostEdit->clear();
  hostEdit->addItems(hosts);
}

QString ConnectDialog::host() const
{
  return hostEdit->currentText();
}

void ConnectDialog::setHost(const QString &host)
{
  hostEdit->setEditText(host);
}

QString ConnectDialog::user() const
{
  return userEdit->text();
}

void ConnectDialog::setUser(const QString &user)
{
  userEdit->setText(user);
}

QString ConnectDialog::pass() const
{
  return passEdit->text();
}

bool ConnectDialog::isPublicServer() const
{
  return publicCheckbox->checkState() == Qt::Checked;
}

void ConnectDialog::setIsPublicServer(bool isPublicServer)
{
  if (isPublicServer) {
    publicCheckbox->setCheckState(Qt::Checked);
    passEdit->setEnabled(false);
  } else {
    publicCheckbox->setCheckState(Qt::Unchecked);
    passEdit->setEnabled(true);
  }
}

void ConnectDialog::loadServerList(const QUrl &url)
{
  serverBrowser->loadServerList(url);
}

void ConnectDialog::onServerSelected(const QString &host)
{
  setHost(host);
  accept();
}

