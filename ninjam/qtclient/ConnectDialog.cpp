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
  layout->addWidget(form);
  layout->addWidget(connectButton);
  setLayout(layout);
  setWindowTitle(tr("Connect to server..."));
}

void ConnectDialog::publicServerStateChanged(int state)
{
  passEdit->setEnabled(state == Qt::Unchecked);
}

QString ConnectDialog::host() const
{
  return hostEdit->text();
}

QString ConnectDialog::user() const
{
  return userEdit->text();
}

QString ConnectDialog::pass() const
{
  return passEdit->text();
}

bool ConnectDialog::isPublicServer() const
{
  return publicCheckbox->checkState() == Qt::Checked;
}
