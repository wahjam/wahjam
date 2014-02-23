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

#include <QApplication>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDomDocument>
#include <QDomNode>

#include "JammrAccessControlDialog.h"

JammrAccessControlDialog::JammrAccessControlDialog(QNetworkAccessManager
    *netManager_, const QUrl &apiUrl_, const QString &server_, QWidget *parent)
  : QDialog(parent), netManager(netManager_), apiUrl(apiUrl_), server(server_)
{
  QVBoxLayout *vBoxLayout = new QVBoxLayout;
  allowRadio = new QRadioButton(tr("Only open to these users"));
  blockRadio = new QRadioButton(tr("Open to everyone except these users"));
  vBoxLayout->addWidget(allowRadio);
  vBoxLayout->addWidget(blockRadio);

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(new QLabel(tr("Usernames:")), 0, 0);
  gridLayout->addWidget(new QLabel(tr("New username:")), 0, 2);

  usernameEdit = new QLineEdit;
  gridLayout->addWidget(usernameEdit, 1, 0);
  addButton = new QPushButton(tr("->"));
  connect(addButton, SIGNAL(clicked()), this, SLOT(addUsername()));
  gridLayout->addWidget(addButton, 1, 1);
  usernamesList = new QListWidget;
  usernamesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  gridLayout->addWidget(usernamesList, 1, 2, 3, 1);

  removeButton = new QPushButton(tr("<-"));
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeUsername()));
  gridLayout->addWidget(removeButton, 3, 1);
  vBoxLayout->addLayout(gridLayout);

  dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Apply |
                                         QDialogButtonBox::Cancel);
  QPushButton *applyButton = dialogButtonBox->button(QDialogButtonBox::Apply);
  connect(applyButton, SIGNAL(clicked()), this, SLOT(applyChanges()));
  connect(dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
  vBoxLayout->addWidget(dialogButtonBox);

  setLayout(vBoxLayout);
  setWindowTitle(tr("Access control..."));

  refresh();
}

void JammrAccessControlDialog::setWidgetsEnabled(bool enable)
{
  allowRadio->setEnabled(enable);
  blockRadio->setEnabled(enable);
  addButton->setEnabled(enable);
  removeButton->setEnabled(enable);
  dialogButtonBox->button(QDialogButtonBox::Apply)->setEnabled(enable);
}

void JammrAccessControlDialog::refresh()
{
  QUrl aclUrl(apiUrl);
  aclUrl.setPath(apiUrl.path() + QString("acls/%2/").arg(server));
  aclUrl.setQuery("format=xml");

  QNetworkRequest request(aclUrl);
  request.setRawHeader("Referer", aclUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());

  qDebug("Fetching access control list at %s",
         aclUrl.toString(QUrl::RemoveUserInfo).toLatin1().constData());

  reply = netManager->get(request);
  connect(reply, SIGNAL(finished()), this, SLOT(completeFetchAcl()));

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  setWidgetsEnabled(false);
}

void JammrAccessControlDialog::completeFetchAcl()
{
  reply->deleteLater();

  QApplication::restoreOverrideCursor();

  QNetworkReply::NetworkError err = reply->error();
  if (err != QNetworkReply::NoError) {
    qCritical("Fetching access control list network reply failed (error=%d)", err);
    QMessageBox::critical(this, tr("Failed to fetch access control list"),
        tr("Unable to fetch access control list due to network failure "
           "(error=%1).  Please ensure you are connected to the internet "
           "and try again.").arg(err));
    reject();
    return;
  }

  qDebug("Finished fetching access control list");

  QDomDocument doc;
  if (!doc.setContent(reply)) {
    qCritical("Access control list XML parse error");
    QMessageBox::critical(this, tr("Failed to fetch access control list"),
        tr("Unable to fetch access control list due to an XML parsing error.  "
           "Please try again later and report this bug if it continues to "
           "happen."));
    reject();
    return;
  }

  QDomNode node(doc.elementsByTagName("mode").item(0));
  QString mode = node.firstChild().nodeValue();
  if (mode == "allow") {
    allowRadio->setChecked(true);
  } else if (mode == "block") {
    blockRadio->setChecked(true);
  } else {
    qCritical("Access control list <mode> XML parsing failed");
    QMessageBox::critical(this, tr("Failed to fetch access control list"),
        tr("Unable to fetch access control list due to missing <mode> XML.  "
           "Please try again later and report this bug if it continues to "
           "happen."));
    reject();
    return;
  }

  usernamesList->clear();
  node = doc.elementsByTagName("usernames").item(0);
  for (QDomNode usernameNode = node.firstChild();
       !usernameNode.isNull();
       usernameNode = usernameNode.nextSibling()) {
    QString username = usernameNode.firstChild().nodeValue();
    if (!username.isEmpty()) {
      usernamesList->addItem(username);
    }
  }
  usernamesList->sortItems();

  setWidgetsEnabled(true);
}

static bool isUsernameValid(const QString &s)
{
  if (s.isEmpty() || s.size() > 30) {
    return false;
  }
  return !s.contains(QRegExp("[^a-zA-Z@+\\.\\-_]"));
}

void JammrAccessControlDialog::addUsername()
{
  QString username = usernameEdit->text();
  if (!isUsernameValid(username)) {
    QMessageBox::critical(this, tr("Invalid username"),
        tr("Usernames must be be 30 characters or less, consisting of "
           "alphanumeric characters or @, +, ., -, _."));
    return;
  }

  if (!usernamesList->findItems(username, Qt::MatchFixedString).empty()) {
    return;
  }

  usernamesList->addItem(username);
  usernamesList->sortItems();
  usernameEdit->clear();
  usernameEdit->setFocus();
}

void JammrAccessControlDialog::removeUsername()
{
  QList<QListWidgetItem *> items = usernamesList->selectedItems();

  foreach (QListWidgetItem *item, items) {
    int row = usernamesList->row(item);
    usernamesList->takeItem(row);
    delete item;
  }
}

void JammrAccessControlDialog::applyChanges()
{
  QUrl aclUrl(apiUrl);
  aclUrl.setPath(apiUrl.path() + QString("acls/%2/").arg(server));

  QNetworkRequest request(aclUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  request.setRawHeader("Referer", aclUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());

  QByteArray formData;
  formData.append(QString("mode=%1").arg(allowRadio->isChecked() ? "allow" : "block"));
  for (int i = 0; i < usernamesList->count(); i++) {
    QListWidgetItem *item = usernamesList->item(i);
    formData.append("&usernames=");
    formData.append(QUrl::toPercentEncoding(item->text()));
  }

  qDebug("Storing access control list at %s",
         aclUrl.toString(QUrl::RemoveUserInfo).toLatin1().constData());

  reply = netManager->post(request, formData);
  connect(reply, SIGNAL(finished()), this, SLOT(completeStoreAcl()));

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  setWidgetsEnabled(false);
}

void JammrAccessControlDialog::completeStoreAcl()
{
  reply->deleteLater();

  QApplication::restoreOverrideCursor();
  setWidgetsEnabled(true);

  QNetworkReply::NetworkError err = reply->error();
  if (err != QNetworkReply::NoError) {
    qCritical("Storing access control list network reply failed (error=%d)", err);
    QMessageBox::critical(this, tr("Failed to store access control list"),
        tr("Unable to store access control list due to network failure "
           "(error=%1).  Please ensure you are connected to the internet "
           "and try again.").arg(err));
    return;
  }

  qDebug("Finished storing access control list");
  accept();
}
