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
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QNetworkRequest>
#include <QCryptographicHash>
#include <QUuid>
#include <QMessageBox>

#include "JammrLoginDialog.h"

JammrLoginDialog::JammrLoginDialog(QNetworkAccessManager *netmanager_,
                                   const QUrl &apiUrl_,
                                   const QUrl &registerUrl, QWidget *parent)
  : QDialog(parent), netmanager(netmanager_), apiUrl(apiUrl_)
{
  userEdit = new QLineEdit;
  passEdit = new QLineEdit;
  passEdit->setEchoMode(QLineEdit::Password);

  QLabel *registerLabel = new QLabel(tr("<a href=\"%1\">Create a new account.</a>").arg(registerUrl.toString()));
  registerLabel->setOpenExternalLinks(true);

  connectButton = new QPushButton(tr("&Log in"));
  connect(connectButton, SIGNAL(clicked()), this, SLOT(login()));

  QVBoxLayout *layout = new QVBoxLayout;
  QWidget *form = new QWidget;
  QFormLayout *formLayout = new QFormLayout;
  formLayout->addRow(tr("&Username:"), userEdit);
  formLayout->addRow(tr("&Password:"), passEdit);
  form->setLayout(formLayout);
  layout->addWidget(form);
  layout->addWidget(registerLabel);
  layout->addWidget(connectButton);
  setLayout(layout);
  setWindowTitle(tr("Log in..."));
}

QString JammrLoginDialog::username() const
{
  return userEdit->text();
}

QString JammrLoginDialog::password() const
{
  return passEdit->text();
}

void JammrLoginDialog::setUsername(const QString &username)
{
  userEdit->setText(username);

  /* Since the username has been filled in the user probably needs to enter
   * their password next.
   */
  passEdit->setFocus();
}

void JammrLoginDialog::setPassword(const QString &password)
{
  passEdit->setText(password);
}

void JammrLoginDialog::login()
{
  /* Generate unique 20-byte token */
  hexToken = QCryptographicHash::hash(QUuid::createUuid().toRfc4122(),
                                      QCryptographicHash::Sha1).toHex();

  QUrl tokenUrl = apiUrl;
  tokenUrl.setPath(tokenUrl.path() + "tokens/" + username() + "/");

  QNetworkRequest request(tokenUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  request.setRawHeader("Referer", tokenUrl.toString().toLatin1().data());
  request.setRawHeader("Authorization", "Basic " +
      (username() + ":" + passEdit->text()).toUtf8().toBase64());
  reply = netmanager->post(request, QString("token=%1").arg(hexToken).toLatin1());
  connect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));

  qDebug("Logging in as \"%s\" at %s", username().toLatin1().data(),
         tokenUrl.toString().toLatin1().data());

  connectButton->setEnabled(false);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void JammrLoginDialog::requestFinished()
{
  reply->deleteLater();

  QApplication::restoreOverrideCursor();

  connectButton->setEnabled(true);

  QNetworkReply::NetworkError err = reply->error();
  if (err != QNetworkReply::NoError) {
    qCritical("Login network reply failed (error=%d)", err);

    QString message;
    switch (err) {
    case QNetworkReply::AuthenticationRequiredError:
      message = tr("Invalid username/password, please try again.");
      break;

    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::UnknownNetworkError:
      message = tr("Unable to login, please check that you are connected to the internet and try again.");
      break;

    default:
      message = tr("Failed to login with error %1, please report this bug.").arg(err);
    }

    QMessageBox::critical(this, tr("Login error"), message);
    return;
  }

  qDebug("Logged in successfully");
  accept();
}

QString JammrLoginDialog::token() const
{
  return hexToken;
}
