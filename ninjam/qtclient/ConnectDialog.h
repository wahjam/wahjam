#ifndef _CONNECTDIALOG_H_
#define _CONNECTDIALOG_H_

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>

class ConnectDialog : public QDialog
{
  Q_OBJECT
  Q_PROPERTY(QString host READ host)
  Q_PROPERTY(QString user READ user)
  Q_PROPERTY(bool isPublicServer READ isPublicServer)
  Q_PROPERTY(QString pass READ pass)

public:
  ConnectDialog(QWidget *parent = 0);
  QString host() const;
  QString user() const;
  bool isPublicServer() const;
  QString pass() const;

private slots:
  void publicServerStateChanged(int state);

private:
  QLineEdit *hostEdit;
  QLineEdit *userEdit;
  QCheckBox *publicCheckbox;
  QLineEdit *passEdit;
};

#endif /* _CONNECTDIALOG_H_ */
