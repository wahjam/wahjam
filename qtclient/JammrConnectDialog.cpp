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
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDomDocument>
#include <QDomNode>

#include "JammrConnectDialog.h"

JammrConnectDialog::JammrConnectDialog(QNetworkAccessManager *netManager_,
                                       const QUrl &apiUrl_,
                                       const QUrl &upgradeUrl_,
                                       QWidget *parent)
  : QDialog(parent), netManager(netManager_), apiUrl(apiUrl_),
    upgradeUrl(upgradeUrl_), reply(NULL)
{
  serverBrowser = new JammrServerBrowser(netManager, this);
  connect(serverBrowser, SIGNAL(serverItemClicked(const QString &)),
          this, SLOT(setHost(const QString &)));
  connect(serverBrowser, SIGNAL(serverItemActivated(const QString &)),
          this, SLOT(onServerSelected(const QString &)));

  connectButton = new QPushButton(tr("&Connect"));
  connect(connectButton, SIGNAL(clicked()), this, SLOT(accept()));

  newJamButton = new QPushButton(tr("&New jam"));
  connect(newJamButton, SIGNAL(clicked()), this, SLOT(createJam()));

  QWidget *buttons = new QWidget;
  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  buttonsLayout->addWidget(newJamButton, 0, Qt::AlignLeft);
  buttonsLayout->addWidget(connectButton, 0, Qt::AlignRight);
  buttons->setLayout(buttonsLayout);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(2);
  layout->setContentsMargins(5, 5, 5, 5);
  layout->addWidget(serverBrowser);
  layout->addWidget(buttons);
  setLayout(layout);
  setWindowTitle(tr("Connect to server..."));
  loadServerList();
}

QString JammrConnectDialog::host() const
{
  return selectedHost;
}

void JammrConnectDialog::setHost(const QString &host)
{
  selectedHost = host;
}

void JammrConnectDialog::onServerSelected(const QString &host)
{
  setHost(host);
  accept();
}

void JammrConnectDialog::loadServerList()
{
  serverBrowser->loadServerList(apiUrl);
}

void JammrConnectDialog::createJam()
{
  QUrl livejamsUrl(apiUrl);
  livejamsUrl.setPath(apiUrl.path() + "livejams/");
  livejamsUrl.addQueryItem("format", "xml");

  QNetworkRequest request(livejamsUrl);
  request.setRawHeader("Referer", livejamsUrl.toString(QUrl::RemoveUserInfo).toAscii().constData());

  /* Cannot use NetworkAccessManager::post() without data to submit */
  reply = netManager->sendCustomRequest(request, "POST");
  connect(reply, SIGNAL(finished()), this, SLOT(createJamFinished()));

  qDebug("Creating jam at %s", livejamsUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());

  connectButton->setEnabled(false);
  newJamButton->setEnabled(false);
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void JammrConnectDialog::createJamFinished()
{
  reply->deleteLater();

  QApplication::restoreOverrideCursor();
  connectButton->setEnabled(true);
  newJamButton->setEnabled(true);

  QNetworkReply::NetworkError err = reply->error();
  if (err != QNetworkReply::NoError) {
    qCritical("Create jam network reply failed (error=%d)", err);
    if (err == QNetworkReply::AuthenticationRequiredError) {
      QMessageBox::critical(this, tr("Failed to create jam"),
          tr("Upgrade to full member <a href=\"%1\">here</a> to "
             "create private jams.").arg(upgradeUrl.toString()));
    } else {
      QMessageBox::critical(this, tr("Failed to create jam"),
          tr("Unable to create jam due to network failure (error=%1).  "
             "Please ensure you are connected to the internet and "
             "try again.").arg(err));
    }
    return;
  }

  QDomDocument doc;
  if (!doc.setContent(reply)) {
    qCritical("Create jam XML parse error");
    QMessageBox::critical(this, tr("Failed to create jam"),
        tr("Unable to create jam due to an XML parsing error.  "
           "Please try again later and report this bug if it "
           "continues to happen."));
    return;
  }

  QDomNode node(doc.elementsByTagName("server").item(0));
  if (node.isNull()) {
    qCritical("Create jam <server> XML parsing failed");
    QMessageBox::critical(this, tr("Failed to create jam"),
        tr("Unable to create jam due to missing <server> XML.  "
           "Please try again later and report this bug if it "
           "continues to happen."));
    return;
  }

  QString server = node.firstChild().nodeValue();
  qDebug("Finished creating jam at %s", server.toLatin1().constData());
  setHost(server);
  accept();
}
