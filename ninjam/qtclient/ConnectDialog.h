#ifndef _CONNECTDIALOG_H_
#define _CONNECTDIALOG_H_

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>

class ConnectDialog : public QDialog
{
  Q_OBJECT
  Q_PROPERTY(QString host READ host)
  Q_PROPERTY(QString user READ user WRITE setUser)
  Q_PROPERTY(bool isPublicServer READ isPublicServer WRITE setIsPublicServer)
  Q_PROPERTY(QString pass READ pass)

public:
  ConnectDialog(QWidget *parent = 0);
  void setRecentHostsList(const QStringList &hosts);
  QString host() const;
  QString user() const;
  void setUser(const QString &user);
  bool isPublicServer() const;
  void setIsPublicServer(bool isPublicServer);
  QString pass() const;

private slots:
  void publicServerStateChanged(int state);

private:
  QComboBox *hostEdit;
  QLineEdit *userEdit;
  QCheckBox *publicCheckbox;
  QLineEdit *passEdit;
};

#endif /* _CONNECTDIALOG_H_ */
