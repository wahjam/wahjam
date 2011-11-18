#include <QLabel>
#include <QFormLayout>

#include "ConnectDialog.h"

ConnectDialog::ConnectDialog(QWidget *parent)
  : QDialog(parent)
{
  hostEdit = new QLineEdit;
  userEdit = new QLineEdit;
  passEdit = new QLineEdit;
  passEdit->setEchoMode(QLineEdit::Password);
  passEdit->setEnabled(false);

  connectButton = new QPushButton(tr("&Connect"));

  connect(connectButton, SIGNAL(clicked()), this, SLOT(connectToHost()));

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
  layout->addWidget(form);
  layout->addWidget(connectButton);
  setLayout(layout);
  setWindowTitle(tr("Connect to server..."));
}

void ConnectDialog::connectToHost()
{
  host = hostEdit->text();
  user = userEdit->text();
  pass = passEdit->text();
  isPublicServer = publicCheckbox->isChecked();

  hostEdit->clear();
  userEdit->clear();
  passEdit->clear();
  passEdit->setEnabled(false);
  publicCheckbox->setChecked(true);

  accept();
}

void ConnectDialog::publicServerStateChanged(int state)
{
  passEdit->setEnabled(state == Qt::Unchecked);
}

QString ConnectDialog::GetHost()
{
  return host;
}

QString ConnectDialog::GetUser()
{
  return user;
}

QString ConnectDialog::GetPass()
{
  return pass;
}

bool ConnectDialog::IsPublicServer()
{
  return isPublicServer;
}
