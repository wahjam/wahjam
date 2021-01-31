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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QKeyEvent>
#include <QTimer>
#include <QUrlQuery>

#include "JammrAccessControlDialog.h"

enum {
  USERNAME_EDIT_MIN_CHARS = 3,
};

JammrAccessControlDialog::JammrAccessControlDialog(QNetworkAccessManager
    *netManager_, const QUrl &apiUrl_, const QString &server_, QWidget *parent)
  : QDialog(parent), netManager(netManager_), apiUrl(apiUrl_), server(server_),
    reply(nullptr), usernamesReply(nullptr), usernameEditTimer(nullptr)
{
  QVBoxLayout *vBoxLayout = new QVBoxLayout;

  vBoxLayout->addWidget(new QLabel(tr("<p>Control who can join this jam session:</p>"
          "<ol><li>Search for users</li>"
          "<li>Double-click on their usernames to add/remove them</li></ol>")));
  vBoxLayout->addSpacing(5);

  QGridLayout *gridLayout = new QGridLayout;

  QRegExpValidator *validator = new QRegExpValidator(QRegExp("[a-zA-Z0-9@\\.+\\-_]{0,30}"), this);
  usernameEdit = new QLineEdit;
  usernameEdit->setPlaceholderText(tr("Username search"));
  usernameEdit->setValidator(validator);
  connect(usernameEdit, &QLineEdit::textChanged,
          this, &JammrAccessControlDialog::usernameEditChanged);
  gridLayout->addWidget(usernameEdit, 0, 0);

  gridLayout->addWidget(new QLabel(tr("User list:")), 0, 1);

  searchList = new QListWidget;
  searchList->setSelectionMode(QAbstractItemView::NoSelection);
  connect(searchList, &QListWidget::itemActivated,
          this, &JammrAccessControlDialog::addUsername);
  gridLayout->addWidget(searchList, 1, 0, 2, 1);

  usernamesList = new QListWidget;
  usernamesList->setSelectionMode(QAbstractItemView::NoSelection);
  usernamesList->setSortingEnabled(true);
  connect(usernamesList, &QListWidget::itemActivated,
          this, &JammrAccessControlDialog::removeUsername);
  gridLayout->addWidget(usernamesList, 1, 1, 2, 1);

  vBoxLayout->addLayout(gridLayout);

  allowRadio = new QRadioButton(tr("Only open to these users"));
  blockRadio = new QRadioButton(tr("Open to everyone except these users"));
  vBoxLayout->addWidget(allowRadio);
  vBoxLayout->addWidget(blockRadio);

  dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Apply |
                                         QDialogButtonBox::Cancel);
  QPushButton *applyButton = dialogButtonBox->button(QDialogButtonBox::Apply);
  connect(applyButton, &QPushButton::clicked,
          this, &JammrAccessControlDialog::applyChanges);
  connect(dialogButtonBox, &QDialogButtonBox::rejected,
          this, &JammrAccessControlDialog::reject);
  vBoxLayout->addWidget(dialogButtonBox);

  setLayout(vBoxLayout);
  setWindowTitle(tr("Access control..."));

  refresh();
}

void JammrAccessControlDialog::setWidgetsEnabled(bool enable)
{
  allowRadio->setEnabled(enable);
  blockRadio->setEnabled(enable);
  searchList->setEnabled(enable);
  usernamesList->setEnabled(enable);
  dialogButtonBox->button(QDialogButtonBox::Apply)->setEnabled(enable);
}

void JammrAccessControlDialog::refresh()
{
  QUrl aclUrl(apiUrl);
  aclUrl.setPath(apiUrl.path() + QString("acls/%2/").arg(server));

  QNetworkRequest request(aclUrl);
  request.setRawHeader("Referer", aclUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());

  qDebug("Fetching access control list at %s",
         aclUrl.toString(QUrl::RemoveUserInfo).toLatin1().constData());

  reply = netManager->get(request);
  connect(reply, &QNetworkReply::finished,
          this, &JammrAccessControlDialog::completeFetchAcl);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  setWidgetsEnabled(false);
}

void JammrAccessControlDialog::completeFetchAcl()
{
  reply->deleteLater();

  QApplication::restoreOverrideCursor();

  QNetworkReply::NetworkError netErr = reply->error();
  if (netErr != QNetworkReply::NoError) {
    qCritical("Fetching access control list network reply failed (error=%d)", netErr);
    QMessageBox::critical(this, tr("Failed to fetch access control list"),
        tr("Unable to fetch access control list due to network failure "
           "(error=%1).  Please ensure you are connected to the internet "
           "and try again.").arg(netErr));
    reject();
    return;
  }

  qDebug("Finished fetching access control list");

  QJsonParseError jsonErr;
  QJsonDocument doc(QJsonDocument::fromJson(reply->readAll(), &jsonErr));
  if (doc.isNull()) {
    qCritical("Access control list JSON parse error: %s",
              jsonErr.errorString().toLatin1().constData());
    QMessageBox::critical(this, tr("Failed to fetch access control list"),
        tr("Unable to fetch access control list due to a JSON parse "
           "error (%1).  Please try again later and report this bug if it "
           "continues to happen.").arg(jsonErr.errorString()));
    reject();
    return;
  }

  QJsonObject obj(doc.object());
  QString mode = obj.value("mode").toString();
  if (mode == "allow") {
    allowRadio->setChecked(true);
  } else if (mode == "block") {
    blockRadio->setChecked(true);
  } else {
    qCritical("Access control list \"mode\" JSON parsing failed");
    QMessageBox::critical(this, tr("Failed to fetch access control list"),
        tr("Unable to fetch access control list due to missing \"mode\" "
           "JSON.  Please try again later and report this bug if it continues "
           "to happen."));
    reject();
    return;
  }

  usernamesList->clear();
  foreach (QJsonValue elem, obj.value("usernames").toArray()) {
    QString username = elem.toString();
    if (!username.isEmpty()) {
      usernamesList->addItem(new QListWidgetItem(QIcon(":/icons/remove.svg"), username));
    }
  }

  setWidgetsEnabled(true);
}

void JammrAccessControlDialog::addUsername(QListWidgetItem *item)
{
  QString username = item->text();

  if (item->text() == tr("More results omitted...")) {
    return;
  }

  if (!usernamesList->findItems(username, Qt::MatchFixedString).empty()) {
    recentSearches_.removeAll(username);
    delete item;
    return;
  }

  if (!recentSearches_.contains(username)) {
    recentSearches_.append(username);
  }

  usernamesList->addItem(new QListWidgetItem(QIcon(":/icons/remove.svg"), username));
}

void JammrAccessControlDialog::removeUsername(QListWidgetItem *item)
{
  delete item;
}

void JammrAccessControlDialog::applyChanges()
{
  if (usernamesReply) {
    usernamesReply->abort();
  }

  QUrl aclUrl(apiUrl);
  aclUrl.setPath(apiUrl.path() + QString("acls/%2/").arg(server));

  QNetworkRequest request(aclUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  request.setRawHeader("Referer", aclUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());

  QByteArray formData;
  formData.append(QString("mode=%1").arg(allowRadio->isChecked() ? "allow" : "block").toUtf8());
  for (int i = 0; i < usernamesList->count(); i++) {
    QListWidgetItem *item = usernamesList->item(i);
    formData.append("&usernames=");
    formData.append(QUrl::toPercentEncoding(item->text()));
  }

  qDebug("Storing access control list at %s",
         aclUrl.toString(QUrl::RemoveUserInfo).toLatin1().constData());

  reply = netManager->post(request, formData);
  connect(reply, &QNetworkReply::finished,
          this, &JammrAccessControlDialog::completeStoreAcl);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  setWidgetsEnabled(false);
}

void JammrAccessControlDialog::completeStoreAcl()
{
  QNetworkReply::NetworkError err = reply->error();
  reply->deleteLater();
  reply = nullptr;

  QApplication::restoreOverrideCursor();
  setWidgetsEnabled(true);

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

QStringList JammrAccessControlDialog::recentSearches() const
{
  return recentSearches_;
}

void JammrAccessControlDialog::showRecentSearches()
{
  searchList->clear();

  for (const QString &username : recentSearches_) {
    searchList->addItem(new QListWidgetItem(QIcon(":/icons/add.svg"), username));
  }
}
void JammrAccessControlDialog::setRecentSearches(const QStringList &usernames)
{
  recentSearches_ = usernames;

  if (usernameEdit->text().size() < USERNAME_EDIT_MIN_CHARS) {
    showRecentSearches();
  }
}

void JammrAccessControlDialog::usernameEditChanged(const QString &text)
{
  if (usernamesReply) {
    usernamesReply->abort();
  }
  if (usernameEditTimer) {
    usernameEditTimer->stop();
  }

  if (text.size() < USERNAME_EDIT_MIN_CHARS) {
    showRecentSearches();
    return;
  }

  if (!usernameEditTimer) {
    usernameEditTimer = new QTimer(this);
    usernameEditTimer->setSingleShot(true);
    connect(usernameEditTimer, &QTimer::timeout,
            this, &JammrAccessControlDialog::usernameEditTimeout);
  }
  usernameEditTimer->start(150 /* ms */);
}

void JammrAccessControlDialog::usernameEditTimeout()
{
  usernameEditTimer->deleteLater();
  usernameEditTimer = nullptr;

  QUrlQuery query;
  query.addQueryItem("q", usernameEdit->text());

  QUrl usernamesUrl(apiUrl);
  usernamesUrl.setPath(apiUrl.path() + QString("usernames/"));
  usernamesUrl.setQuery(query);

  QNetworkRequest request(usernamesUrl);
  request.setRawHeader("Referer", usernamesUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());

  qDebug("Fetching usernames at %s",
         usernamesUrl.toString(QUrl::RemoveUserInfo).toLatin1().constData());

  usernamesReply = netManager->get(request);
  connect(usernamesReply, &QNetworkReply::finished,
          this, &JammrAccessControlDialog::completeUsernameSearch);
}

void JammrAccessControlDialog::completeUsernameSearch()
{
  QNetworkReply *theReply = usernamesReply;
  usernamesReply->deleteLater();
  usernamesReply = nullptr;

  QNetworkReply::NetworkError err = theReply->error();
  if (err == QNetworkReply::OperationCanceledError) {
    return;
  } else if (err != QNetworkReply::NoError) {
    qCritical("Fetching usernames network reply failed (error=%d)", err);
    return;
  }

  QJsonParseError jsonErr;
  QJsonDocument doc(QJsonDocument::fromJson(theReply->readAll(), &jsonErr));
  if (doc.isNull()) {
    qCritical("Fetching usernames JSON parse error: %s",
              jsonErr.errorString().toLatin1().constData());
    return;
  }

  QStringList usernames = doc.object().value("usernames").toVariant().toStringList();
  qDebug("Got usernames: %s (has_more=%d)",
         usernames.join(",").toLatin1().constData(),
         doc.object().value("has_more").toBool());

  searchList->clear();
  for (const QString &username : usernames) {
    searchList->addItem(new QListWidgetItem(QIcon(":/icons/add.svg"), username));
  }
  searchList->sortItems();

  if (doc.object().value("has_more").toBool()) {
    QListWidgetItem *item = new QListWidgetItem(tr("More results omitted..."));
    QFont font = item->font();
    font.setItalic(true);
    item->setFont(font);
    searchList->addItem(item);
  }
}

/* By default the OK button is activated when the user presses Enter in the
 * username text box. Suppress this.
 */
void JammrAccessControlDialog::keyPressEvent(QKeyEvent *event)
{
  int key = event->key();

  if (QApplication::focusWidget() == usernameEdit &&
      (key == Qt::Key_Return || key == Qt::Key_Enter)) {
    event->ignore();
  } else {
    QDialog::keyPressEvent(event);
  }
}
