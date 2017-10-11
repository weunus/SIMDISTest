#ifndef SOCKET_SIMDIS_H
#define SOCKET_SIMDIS_H 1

#include <QUdpSocket>
#include <QTcpServer>
#include "dlps/MHITypes.h"
#include "dlps/TDBRecord.h"
#include "DLPSBufferHandler.h"
#include "DCSDefinition.h"
#include <qtimer.h>
#include "qvector3d.h"
#include <vector>

using namespace std;

typedef signed __int8     int8_t;

enum SIMDISStatus
{
	SIMDISStatusWait,
	SIMDISStatusConnecting,
	SIMDISStatusConnected,
	SIMDISStatusDisconnected,
	SIMDISStatusNewTrack,
	SIMDISStatusUpdateTrack,
	SIMDISStatusDropTrack,
	SIMDISStatusSendMissile
};

struct TrackInfo
{
	DLPSString iconname;
	
	double latitude;
	double longitude;
	double altitude;	// meter
	
	double positionX;
	double positionY;
	double positionZ;

	double orientationX;	// Yaw
	double orientationY;	// Pitch
	//double orientationZ;	// Roll

	TDBRecord record;

	TrackInfo()
	{
		latitude = 0.0;
		longitude = 0.0;
		altitude = 0.0;

		positionX = 0.0;
		positionY = 0.0;
		positionZ = 0.0;

		orientationX = 0.0;
		orientationY = 0.0;
		//orientationZ = 0.0;
	}
};

struct EventData
{
	double time;
	unsigned int data_;
	char charData[128];
	int8_t eventType;
};

class CreateHeaderContainer;
class SocketSIMDIS : public QTcpServer
{
	Q_OBJECT

public:
	SocketSIMDIS();

	bool StartServer(QString clientRequestAddr, int clientRequestPort,
					 QString serverResponseAddr, int serverResponsePort, QString serverName,
					 QString serverAddr, int serverPort,
					 QString simdisAddr, int simdisPort);

	void StopServer();
	void SendMissileData(double lat, double lon, double alt, double vx, double vy, double vz);
	void SendTrackMessage(MHIMessageID id, const void* recordBuffer, unsigned int length);

signals:
	void sigChangeStatus(SIMDISStatus, QString msg="");
	
private slots:
	void OnReadyReadClientRequest();
	void OnReadyRead();

	void incomingConnection(qintptr handle);
	void OnDisconnected();

private:
	void WriteDouble(DLPSBufferHandler& handler, double value);
	void WriteCharacters(DLPSBufferHandler& handler, int length, const char* characters);

	void CreatePlatformHeader(DLPSBufferHandler& handler, unsigned long long tn, TrackInfo info, const char* callsign);
	void CreatePlatformData(DLPSBufferHandler& handler, unsigned long long tn, TrackInfo& orgInfo, double latitude = 0.0, double longitude = 0.0, double altitude = 0.0, bool update = false);

	void CreateCategoryData(DLPSBufferHandler& handler, unsigned long long tn, QString name, QString value);
	
	void CreateBeamHeader(DLPSBufferHandler& handler, unsigned long long tn, TrackInfo info, unsigned long long targetTN, int beamType, bool bTargetTN = false, bool bMissile = false);
	void CreateBeamData(DLPSBufferHandler& handler, unsigned long long tn);

	void CreateEvent(DLPSBufferHandler& handler, unsigned long long id, unsigned long long data, char* charData, __int8 eventType);
	
	void WriteBase(DLPSBufferHandler& handler, DCSDataType_t type, unsigned long long id);
	void WriteReferenceFrame(DLPSBufferHandler& handler, double latitude, double longitude, double altitude);

	void SendAllCategoryData(QTcpSocket* client, unsigned long long tn, TDBRecord record);

	void SendDropMessage(unsigned long long target, unsigned long long missileTN);

	void GetTargetLatLong(double sourceLat, double sourceLong,
		double bearing, double distance,
		double &destLat, double &destLong);



public slots:
	void DrawMissile();

private:
	QString			_serverName;

	QHostAddress	_clientRequestAddr;
	int				_clientRequestPort;
	QUdpSocket		_clientRequestSocket;

	QHostAddress	_serverResponseAddr;
	int				_serverResponsePort;
	QUdpSocket		_serverResponseSocket;
	QString			_strResponse;

	QHostAddress	_serverAddr;
	int				_serverPort;


	QHostAddress	_simdisAddr;
	int				_simdisPort;

	vector<QTcpSocket*> _clients;
	map<unsigned long long /*tn*/, TrackInfo> _tracks;
	map<unsigned long long /*tn*/, TrackInfo> _missile;

	char	_serverID[DCS_SERVERIDSZ];

	int m_beamId = 0;

	QVector3D _StartPos;  // x : Lat, y : Lng, z : Alt 
	QVector3D _EndPos;
	//QVector3D _CurrentPos;

	double m_vx;
	double m_vy;
	double m_vz;
	double m_g; //z축으로의 중력가속도

	QTimer m_timerSendMissile;
	//bool m_bFirstMessage;

	//unsigned long long m_missileKey;
	unsigned long long m_objectTN;

	CreateHeaderContainer* m_container;
};

#endif //SOCKET_SIMDIS_H