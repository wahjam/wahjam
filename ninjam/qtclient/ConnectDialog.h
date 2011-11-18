#ifndef _CONNECTDIALOG_H_
#define _CONNECTDIALOG_H_

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>

class ConnectDialog : public QDialog
{
	Q_OBJECT

public:
	ConnectDialog(QWidget *parent = 0);
	QString GetHost();
	QString GetUser();
	QString GetPass();
	bool IsPublicServer();

public slots:
	void connectToHost();
	void publicServerStateChanged(int state);

private:
	QLineEdit *hostEdit;
	QLineEdit *userEdit;
	QCheckBox *publicCheckbox;
	QLineEdit *passEdit;
	QPushButton *connectButton;
	QString host;
	QString user;
	QString pass;
	bool isPublicServer;
};

#endif /* _CONNECTDIALOG_H_ */
