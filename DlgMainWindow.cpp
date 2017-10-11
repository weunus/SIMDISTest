#include "DlgMainWindow.h"
#include "DLPSBufferHandler.h"

#define HP_TACT_IP				"225.1.249.1"
#define HP_TACT_PORT			10011
#define REQUEST_RESPONSE_IP		"227.4.4.25"
#define REQUEST_PORT			25252
#define RESPONSE_PORT			26262
#define SERVER_NAME				"WEUNUS"
#define SERVER_IP				"192.168.100.34"
#define SERVER_PORT				2525
#define SIMDIS_IP				"227.0.20.20"
#define SIMDIS_PORT				2020

DlgMainWindow::DlgMainWindow(QWidget *parent)
: QMainWindow(parent),
_joinedHP(false),
_startedServer(false),
m_updateCnt(0)
{
	_ui.setupUi(this);
	setWindowTitle(QString::fromLocal8Bit("SIMDIS 에뮬레이터"));
	_ui.progressBar->setValue(0);

	connect(_ui.btnInitialize, SIGNAL(clicked()), this, SLOT(OnClickInitialize()));
	connect(&_initDLPS, SIGNAL(sigInitialize(int, QString)), this, SLOT(OnInitialize(int, QString)));

	connect(_ui.btnJoinLeaveHP, SIGNAL(clicked()), this, SLOT(OnClickMultiJoinLeaveHP()));
	connect(_ui.btnStartStopServer, SIGNAL(clicked()), this, SLOT(OnClickStartStopServer()));
	connect(&_socketDLPS, SIGNAL(sigReceiveMessage(const ICDMessageHeader*, const MHIMessageSubheader*, const char*, const void*, unsigned int)),
		this, SLOT(OnReceiveMessage(const ICDMessageHeader*, const MHIMessageSubheader*, const char*, const void*, unsigned int)));
	connect(&_socketSIMDIS, SIGNAL(sigChangeStatus(SIMDISStatus, QString)), this, SLOT(OnChangeStatus(SIMDISStatus, QString)));

	_ui.edtHPIP->setText(HP_TACT_IP);
	_ui.edtHPPort->setText(QString::number(HP_TACT_PORT));
	_ui.edtClientRequestIP->setText(REQUEST_RESPONSE_IP);
	_ui.edtClientRequestPort->setText(QString::number(REQUEST_PORT));
	_ui.edtServerResponseIP->setText(REQUEST_RESPONSE_IP);
	_ui.edtServerResponsePort->setText(QString::number(RESPONSE_PORT));
	_ui.edtServerName->setText(SERVER_NAME);
	_ui.edtServerIP->setText(SERVER_IP);
	_ui.edtServerPort->setText(QString::number(SERVER_PORT));
	_ui.edtSIMDISIP->setText(SIMDIS_IP);
	_ui.edtSIMDISPort->setText(QString::number(SIMDIS_PORT));

	_ui.btnJoinLeaveHP->setDisabled(true);
	m_updateCnt = 0; 
	OnClickInitialize();
	OnClickStartStopServer();
}

DlgMainWindow::~DlgMainWindow()
{
}
void DlgMainWindow::OnClickInitialize()
{
	_ui.btnInitialize->setDisabled(true);
	_ui.edtLog->append(QString::fromLocal8Bit("DLPS 라이브러리 초기화 중...."));
	_initDLPS.start();
}
void DlgMainWindow::OnInitialize(int progress, QString strErr)
{
	if (strErr.isEmpty())
	{
		if (progress < 100)
		{
			_ui.progressBar->setValue(progress);
		}
		else
		{
			_ui.progressBar->hide();
			_ui.btnInitialize->hide();
			_ui.edtLog->append(QString::fromLocal8Bit("초기화 성공"));
			_ui.btnJoinLeaveHP->setDisabled(false);
			OnClickMultiJoinLeaveHP();
		}
	}
	else
	{
		_ui.btnInitialize->setDisabled(false);
		_ui.edtLog->append(QString::fromLocal8Bit("초기화 실패\n"));
		_ui.edtLog->append(strErr);
	}
}

void DlgMainWindow::OnClickMultiJoinLeaveHP()
{
	if (!_joinedHP)
	{
		QString addr = _ui.edtHPIP->text();
		int port = _ui.edtHPPort->text().toUInt();
		_joinedHP = _socketDLPS.Join(addr, port);
		if (_joinedHP)
		{
			_ui.edtHPIP->setDisabled(true);
			_ui.edtHPPort->setDisabled(true);
			_ui.btnJoinLeaveHP->setText("Leave");

			// TO DO
			_ui.btnJoinLeaveHP->setDisabled(true);
		}
	}
	else
	{
		//_joinedHP = !_socketDLPS.Leave();
		if (!_joinedHP)
		{
			_ui.edtHPIP->setDisabled(false);
			_ui.edtHPPort->setDisabled(false);
			_ui.btnJoinLeaveHP->setText("Join");
		}
	}
}

void DlgMainWindow::OnClickStartStopServer()
{
	if (!_startedServer)
	{
		_startedServer = _socketSIMDIS.StartServer(_ui.edtClientRequestIP->text(),
												   _ui.edtClientRequestPort->text().toUInt(),
												   _ui.edtServerResponseIP->text(),
												   _ui.edtServerResponsePort->text().toUInt(),
												   _ui.edtServerName->text(),
												   _ui.edtServerIP->text(),
												   _ui.edtServerPort->text().toUInt(),
												   _ui.edtSIMDISIP->text(),
												   _ui.edtSIMDISPort->text().toUInt());
		if (_startedServer)
		{
			_ui.edtClientRequestIP->setDisabled(true);
			_ui.edtClientRequestPort->setDisabled(true);
			_ui.edtServerResponseIP->setDisabled(true);
			_ui.edtServerResponsePort->setDisabled(true);
			_ui.edtServerName->setDisabled(true);
			_ui.edtServerIP->setDisabled(true);
			_ui.edtServerPort->setDisabled(true);
			_ui.edtSIMDISIP->setDisabled(true);
			_ui.edtSIMDISPort->setDisabled(true);
			_ui.btnStartStopServer->setText("Stop");

			// TO DO
			_ui.btnStartStopServer->setDisabled(true);
		}
	}
	else
	{
		_socketSIMDIS.StopServer();
		_startedServer = false;
		_ui.edtClientRequestIP->setDisabled(false);
		_ui.edtClientRequestPort->setDisabled(false);
		_ui.edtServerResponseIP->setDisabled(false);
		_ui.edtServerResponsePort->setDisabled(false);
		_ui.edtServerName->setDisabled(false);
		_ui.edtServerIP->setDisabled(false);
		_ui.edtServerPort->setDisabled(false);
		_ui.edtSIMDISIP->setDisabled(false);
		_ui.edtSIMDISPort->setDisabled(false);
		_ui.btnStartStopServer->setText("Start");
	}
}
void DlgMainWindow::OnChangeStatus(SIMDISStatus status, QString strMsg)
{
	QString output;
	switch (status)
	{
	case SIMDISStatusWait:				output = QString::fromLocal8Bit("클라이언트 접속 대기중..."); break;
	case SIMDISStatusConnecting:		output = QString::fromLocal8Bit("클라이언트 접속중..."); break;
	case SIMDISStatusConnected:			output = QString::fromLocal8Bit("클라이언트 접속 완료"); break;
	case SIMDISStatusDisconnected:		output = QString::fromLocal8Bit("클라이언트 접속 종료"); break;
	case SIMDISStatusNewTrack:			output = QString::fromLocal8Bit("New Track"); break;
	case SIMDISStatusUpdateTrack:		output = QString::fromLocal8Bit("Update Track"); break;
	case SIMDISStatusDropTrack:			output = QString::fromLocal8Bit("Drop Track"); break;
	case SIMDISStatusSendMissile:		output = QString::fromLocal8Bit("Send Missile"); break;
	}

	if (!strMsg.isEmpty())
	{
		output += " [";
		output += strMsg;
		output += "]";
	}

	_ui.edtLog->append(output);
}
void DlgMainWindow::OnReceiveMessage(const ICDMessageHeader *header, const MHIMessageSubheader *subheader,
									 const char *log,
									 const void *record, unsigned int length)
{
	if (header->type == ICDMessageTypeMDLIPHITactical)
	{
		_socketSIMDIS.SendTrackMessage(subheader->messageID, record, length);
	}
}

