/*
    Copyright (C) 2012-2017 Stefan Hajnoczi <stefanha@gmail.com>

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
                                   const QUrl &registerUrl,
                                   const QUrl &resetPasswordUrl,
                                   QWidget *parent)
  : QDialog(parent), netmanager(netmanager_), apiUrl(apiUrl_)
{
  userEdit = new QLineEdit;
  passEdit = new QLineEdit;
  passEdit->setEchoMode(QLineEdit::Password);

  rememberPasswordCheckBox = new QCheckBox(tr("Remember password"));

  QLabel *resetPasswordLabel = new QLabel(tr("<a href=\"%1\">Reset password.</a>").arg(resetPasswordUrl.toString()));
  resetPasswordLabel->setOpenExternalLinks(true);

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
  layout->addWidget(rememberPasswordCheckBox);
  layout->addWidget(resetPasswordLabel);
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

bool JammrLoginDialog::rememberPassword() const
{
  return rememberPasswordCheckBox->isChecked();
}

void JammrLoginDialog::setUsername(const QString &username)
{
  userEdit->setText(username);

  /* Since the username has been filled in the user probably needs to enter
   * their password next.
   */
  if (!username.isEmpty()) {
    passEdit->setFocus();
  }
}

void JammrLoginDialog::setPassword(const QString &password)
{
  passEdit->setText(password);
}

void JammrLoginDialog::setRememberPassword(bool value)
{
  rememberPasswordCheckBox->setChecked(value);
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
  connect(reply, &QNetworkReply::sslErrors,
          this, &JammrLoginDialog::requestSslErrors);

  qDebug("Logging in as \"%s\" at %s", username().toLatin1().data(),
         tokenUrl.toString().toLatin1().data());

  connectButton->setEnabled(false);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void JammrLoginDialog::requestSslErrors(const QList<QSslError> &errors)
{
  for (const QSslError &error : errors) {
    qCritical("SSL error: %s", error.errorString().toLatin1().constData());
  }
}

void JammrLoginDialog::requestFinished()
{
  reply->deleteLater();

  QApplication::restoreOverrideCursor();

  connectButton->setEnabled(true);

  QNetworkReply::NetworkError err = reply->error();
  if (err != QNetworkReply::NoError) {
    qCritical("Login network reply failed (error=%d, errorString=\"%s\")",
              err, reply->errorString().toLatin1().constData());

    QString message;
    switch (err) {
    case QNetworkReply::AuthenticationRequiredError:
      if (username().contains('@')) {
        message = tr("Invalid username/password.  Your username may be different from your email address.  Please try again with your username.");
      } else {
        message = tr("Invalid username/password, please try again.");
      }
      break;

    case QNetworkReply::SslHandshakeFailedError:
      message = tr("SSL handshake failed, please check that your computer's date and time are accurate.");
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
