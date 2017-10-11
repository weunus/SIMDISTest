#ifndef SIMDISTEST_H
#define SIMDISTEST_H

#include <QtWidgets/QMainWindow>
#include <qtimer.h>
#include "ui_SIMDISTest.h"
#include "InitDLPS.h"
#include "SocketDLPS.h"
#include "SocketSIMDIS.h"
#include "qvector3d.h"

class DlgMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	DlgMainWindow(QWidget *parent = 0);
	~DlgMainWindow();

public slots:
	void OnClickInitialize();
	void OnInitialize(int, QString);

	void OnClickMultiJoinLeaveHP();
	void OnClickStartStopServer();
	void OnChangeStatus(SIMDISStatus, QString);

	void OnReceiveMessage(const ICDMessageHeader *header, const MHIMessageSubheader *subheader,
						  const char *log,
						  const void *record, unsigned int length);
private:
	Ui::SIMDISTestClass _ui;

	InitDLPS _initDLPS;
	SocketDLPS _socketDLPS;
	SocketSIMDIS _socketSIMDIS;

	bool _joinedHP;
	bool _startedServer;

	int m_updateCnt; 
};

#endif // SIMDISTEST_H
