#include "SocketSIMDIS.h"
#include <QApplication>
#include <QTcpSocket>
#include "dlps/TDBField.h"
#include "dlps/JReferenceTN.h"
#include "LKDataElemDict.h"
#include "JDataElem.h"
#include "JDataElemKey.h"
#include "JDataItemSet.h"
#include "JDataItem.h"
#include "Utils.h"
#include "CreateHeaderContainer.h"

#define ANNOUNCEMENT_SERVER_TO_CLIENT "HELO_DCS_CLIENT"
#define ANNOUNCEMENT_CLIENT_TO_SERVER "HELO_DCS_SERVER"

#define SERVER_ID 10
#define DCS_VERSION 0x0204
#define OWN_LAT 36.0
#define OWN_LON 127.0
#define OWN_ALT 0.0

#define MAX_DATAGRAM_SIZE 16384

#define ADD_KEY_FOR_MISSILE 10000000 //

SocketSIMDIS::SocketSIMDIS()
: _clientRequestPort(0)
, _serverResponsePort(0)
, _serverPort(0)
, _simdisPort(0)
, m_vx(0.0)
, m_vy(0.0)
, m_vz(0.0)
{
	memset(_serverID, 0x00, DCS_SERVERIDSZ);
	moveToThread(QApplication::instance()->thread());
	srand((unsigned int)time(NULL));

	connect(&m_timerSendMissile, SIGNAL(timeout()), this, SLOT(DrawMissile()));
	//m_bFirstMessage = true;
}

bool SocketSIMDIS::StartServer(QString clientRequestAddr, int clientRequestPort,
	QString serverResponseAddr, int serverResponsePort, QString serverName,
	QString serverAddr, int serverPort,
	QString simdisAddr, int simdisPort)
{
	bool success = false;


	_clientRequestAddr = clientRequestAddr;
	_clientRequestPort = clientRequestPort;
	_serverResponseAddr = serverResponseAddr;
	_serverResponsePort = serverResponsePort;
	_serverAddr = serverAddr;
	_serverPort = serverPort;
	_serverName = serverName;
	_simdisAddr = simdisAddr;
	_simdisPort = simdisPort;

	// TO DO
	uint addr = _serverAddr.toIPv4Address(); // 577022144;	// TO DO
	int port = _serverPort;// 56585;
	int serverTime = 0;
	memcpy(_serverID, &addr, 4);
	memcpy(_serverID + 4, &port, 2);
	memcpy(_serverID + 6, &serverTime, 4);

	if (_serverName.isEmpty())
	{
		_serverName = "WEUNUS";
	}

	_strResponse = QString("%1 %2 %3 %4").arg(ANNOUNCEMENT_SERVER_TO_CLIENT).arg(serverAddr).arg(QString::number(serverPort)).arg(_serverName);

	// START SERVER
	if (listen(_serverAddr, _serverPort))
	{
		// JOIN CLIENT REQUEST
		if (_clientRequestSocket.bind(QHostAddress::AnyIPv4, _clientRequestPort, QUdpSocket::ReuseAddressHint) &&
			_clientRequestSocket.joinMulticastGroup(_clientRequestAddr))
		{
			connect(&_clientRequestSocket, SIGNAL(readyRead()), this, SLOT(OnReadyReadClientRequest()));
			emit sigChangeStatus(SIMDISStatusWait);
			success = true;
		}

		//
		// reload DCS database file (if file exists)
		if (success)
		{
			int id = 0;
			m_container = new CreateHeaderContainer();
			if (m_container->Restore("dcsdb.dat"))
			{

				id = static_cast<int>(m_container->size() / 3);
				//sstr.str("");
				//sstr << "Loaded " << headers.size() << "platforms from file\n";
				//logger.log(sstr.str().c_str());
			}
			else
			{
				m_container->Initialize("dcsdb.dat");
			}
		}
		//
	}
	return success;
}

void SocketSIMDIS::StopServer()
{
	_clients.clear();
	_tracks.clear();

	// TO DO
}

void SocketSIMDIS::OnReadyReadClientRequest()
{
	qint64 recvSize;
	char *message;
	while (_clientRequestSocket.hasPendingDatagrams())
	{
		recvSize = _clientRequestSocket.pendingDatagramSize();
		message = new char[recvSize];
		if ((_clientRequestSocket.readDatagram(message, recvSize)) != -1)
		{
			if (QString(ANNOUNCEMENT_CLIENT_TO_SERVER) == message)
			{
				QUdpSocket().writeDatagram(_strResponse.toLocal8Bit().data(), _strResponse.length() + 1, _serverResponseAddr, _serverResponsePort);
			}
		}
		delete[] message;
	}
}
void SocketSIMDIS::OnReadyRead()
{
	QTcpSocket *client = static_cast<QTcpSocket*>(sender());
	while (client->bytesAvailable() > 0)
	{
		QByteArray recvMsg = client->readAll();
		while (!recvMsg.isEmpty())
		{
			DCSRequestType requestType = (DCSRequestType)(int)recvMsg[0];

			char buffer[MAX_DATAGRAM_SIZE];
			DLPSBufferHandler handler(buffer);

			if (requestType == DCSREQUEST_CONNECT)
			{
				DLPSWriteChar(handler, DCSREQUEST_CONNECT);
			}
			else if (requestType == DCSREQUEST_VERSION)
			{
				DLPSWriteShort(handler, DCS_VERSION);			// version
			}
			else if (requestType == DCSREQUEST_DATAMODE)
			{
				DLPSWriteChar(handler, DCSREQUEST_DATAMODE);
				DLPSWriteChar(handler, DCSDATA_MULTICAST);
				DLPSWriteShort(handler, _simdisPort);
				WriteCharacters(handler, 16, _simdisAddr.toString().toLocal8Bit().data());
				WriteCharacters(handler, DCS_SERVERIDSZ, _serverID);
			}
			else if (requestType == DCSREQUEST_TIME)
			{
				DLPSWriteChar(handler, DCSREQUEST_TIME);
				WriteBase(handler, DCSTIMEHEADER, SERVER_ID);

				// DCSTimeHeader (9)
				WriteDouble(handler, 1.0);				// realtimemod_
				DLPSWriteChar(handler, 1);				// timetype_

				// DCSTimeData (20)
				WriteBase(handler, DCSTIMEDATA, SERVER_ID);
				WriteDouble(handler, yeartime());				// time_
			}
			else if (requestType == DCSREQUEST_SCENARIO)
			{
				DLPSWriteChar(handler, DCSREQUEST_SCENARIO);

				WriteBase(handler, DCSSCENARIOHEADER, SERVER_ID);
				WriteReferenceFrame(handler, OWN_LAT, OWN_LON, OWN_ALT);

				// year (2)
				DLPSWriteShort(handler, 2017);							// referenceyear_

				// (64)
				WriteCharacters(handler, 64, _serverName.toLocal8Bit().data());					// classificationstr_

				// (4)
				DLPSWriteInteger(handler, 2147548928);					// classcolor_

				// DCSScenarioData (28)
				WriteBase(handler, DCSSCENARIODATA, SERVER_ID);
				WriteDouble(handler, 0.0);								// winddirection_
				WriteDouble(handler, 0.0);								// windspeed_
			}
			else if (requestType == DCSREQUEST_HEADERS)
			{
				DLPSWriteChar(handler, DCSREQUEST_HEADERS);
				DLPSWriteInteger(handler, _tracks.size());
			}

			if (handler.GetByteOffset())
			{
				int len = client->write(buffer, handler.GetByteOffset());
				if (requestType == DCSREQUEST_HEADERS)
				{
					for (auto& track : _tracks)
					{
						char bufferTrack[MAX_DATAGRAM_SIZE];
						DLPSBufferHandler handlerTrack(bufferTrack);
						CreatePlatformHeader(handlerTrack, track.first, track.second, J_REFERENCE_TN_TO_TEXT(track.first));
						client->write(bufferTrack, handlerTrack.GetByteOffset());
					}

					emit sigChangeStatus(SIMDISStatusConnected, QString("%1").arg(QString::number((int)client)));
				}
			}
			recvMsg.remove(0, 1);
		}
	}
}

void SocketSIMDIS::incomingConnection(qintptr handle)
{
	QTcpSocket* client = new QTcpSocket(this);
	emit sigChangeStatus(SIMDISStatusConnecting, QString("%1").arg(QString::number((int)client)));

	client->setSocketDescriptor(handle);
	_clients.push_back(client);

	connect(client, SIGNAL(readyRead()), this, SLOT(OnReadyRead()));
	connect(client, SIGNAL(disconnected()), this, SLOT(OnDisconnected()));
}
void SocketSIMDIS::OnDisconnected()
{
	QTcpSocket* client = (QTcpSocket*)sender();
	emit sigChangeStatus(SIMDISStatusDisconnected, QString("%1").arg(QString::number((int)client)));

	vector<QTcpSocket*>::iterator iter = _clients.begin();
	while (iter != _clients.end())
	{
		if (*iter == client)
		{
			_clients.erase(iter);
			return;
		}
		iter++;
	}
}

//void SocketSIMDIS::SendTrackMessage(MHIMessageID id, const void* recordBuffer, unsigned int length)

double dTime = 0.f; //흐르는 시간
void SocketSIMDIS::SendMissileData(double lat, double lon, double alt, double vx, double vy, double vz)
{
	char buffer[MAX_DATAGRAM_SIZE];
	DLPSBufferHandler handler(buffer);
	//unsigned long long tn = m_objectTN;
	unsigned long long missileTN = ADD_KEY_FOR_MISSILE + m_objectTN;

	bool first = false;
	//map<unsigned long long, TrackInfo>::iterator iter = _tracks.find(tn);
	map<unsigned long long, TrackInfo>::iterator iter = _missile.find(missileTN);
	if (iter == _missile.end())
	{
		first = true;
	}

	m_vx = vx;
	m_vy = vy;
	m_vz = vz;

	if (first)
	{
		//emit sigChangeStatus(SIMDISStatusNewTrack, QString("%1").arg(J_REFERENCE_TN_TO_TEXT(tn)));
		emit sigChangeStatus(SIMDISStatusSendMissile, QString("%1").arg(missileTN));

		TrackInfo info;
		//info.identity = identity;
		info.latitude = lat;
		info.longitude = lon;
		info.altitude = alt;

		//info.record.SetBuffer(record.GetBuffer(), record.GetBufferSize());

		info.iconname = strMissiles[0];
		//_tracks[tn] = info;
		_missile[missileTN] = info;

		if (_clients.size())
		{
			DLPSWriteChar(handler, DCSGATEDATA);
			CreatePlatformHeader(handler, missileTN, info, "Missile");
			for (auto& client : _clients)
			{
				client->write(buffer, handler.GetByteOffset());
				//SendAllCategoryData(client, tn, info.record);
			}
		}
	}
	else
	{
		emit sigChangeStatus(SIMDISStatusSendMissile, QString("%1").arg(J_REFERENCE_TN_TO_TEXT(missileTN)));

		if (_clients.size())
		{
			WriteCharacters(handler, DCS_SERVERIDSZ, _serverID);
			//CreatePlatformData(handler, tn, iter->second, lat, lon, alt, true);
			CreatePlatformData(handler, missileTN, iter->second, lat, lon, alt, true);
			QUdpSocket().writeDatagram(buffer, handler.GetByteOffset(), _simdisAddr, _simdisPort);
		}
	}
}

#define TRACK_MANAGEMENT_ACTION_DROP 0
void SocketSIMDIS::SendTrackMessage(MHIMessageID id, const void* recordBuffer, unsigned int length)
{
	TDBRecord record;
	if (record.SetBuffer(recordBuffer, length))
	{
		char buffer[MAX_DATAGRAM_SIZE];
		DLPSBufferHandler handler(buffer);

		char beamBuffer[MAX_DATAGRAM_SIZE];
		DLPSBufferHandler beamHandler(beamBuffer);

		char eventBuffer[MAX_DATAGRAM_SIZE];
		DLPSBufferHandler eventHandler(eventBuffer);

		unsigned long long tn = 0;
		if (record.GetFieldCode("TRACK NUMBER, REFERENCE", tn))
		{
			//DCSDataType_t type;
			if (id == MHIMessageIDAirTrack ||
				id == MHIMessageIDSurfaceTrack ||
				id == MHIMessageIDSubsurfaceTrack ||
				id == MHIMessageIDLandPointTrack)
			{
				double latitude = atof(GetValue(record, "LATITUDE").GetData());
				double longitude = atof(GetValue(record, "LONGITUDE").GetData());
				double altitude = 0.0;
				double course = atof(GetValue(record, "COURSE").GetData());
				//record.GetFieldCode("IDENTITY", identity);

				switch (id)
				{
				case MHIMessageIDAirTrack:			altitude = ConvertFeetToMeters(atof(GetValue(record, "ALTITUDE").GetData())); break;
				case MHIMessageIDSurfaceTrack:
				case MHIMessageIDLandPointTrack:	altitude = ConvertFeetToMeters(atof(GetValue(record, "ELEVATION").GetData())); break;
				case MHIMessageIDSubsurfaceTrack:	altitude = atof(GetValue(record, "DEPTH").GetData()) * -1; break;
				}

				bool first = false;
				map<unsigned long long, TrackInfo>::iterator iter = _tracks.find(tn);
				if (iter == _tracks.end())
				{
					first = true;
				}

				//Beam
				bool bTargetTN = false;
				bool bMissile = true;
				unsigned long long targetTN = -1; //No-Target
				if (bTargetTN = record.GetFieldCode("TRACK NUMBER, TARGET", targetTN))
				{
					bTargetTN = true;
					bMissile = true;
				}

				if (first)
				{
					emit sigChangeStatus(SIMDISStatusNewTrack, QString("%1").arg(J_REFERENCE_TN_TO_TEXT(tn)));

					TrackInfo info;
					//info.identity = identity;
					info.latitude = latitude;
					info.longitude = longitude;
					info.altitude = altitude;
					info.record.SetBuffer(record.GetBuffer(), record.GetBufferSize());

					int index = rand() % CNT_MODEL_NAME;
					switch (id)
					{
					case MHIMessageIDAirTrack:			info.iconname = strAirCraft[index]; break;
					case MHIMessageIDSurfaceTrack:
					case MHIMessageIDSubsurfaceTrack:	info.iconname = strShip[index]; break;
					case MHIMessageIDLandPointTrack:	info.iconname = strVehicles[index]; break;
					}

					_tracks[tn] = info;
					if (_clients.size())
					{
						DLPSWriteChar(handler, DCSGATEDATA);
						CreatePlatformHeader(handler, tn, info, (char*)J_REFERENCE_TN_TO_TEXT(tn));

						//m_container->Insert(handler, tn);

						m_beamId++;//m_beamId += 10; 
						DLPSWriteChar(beamHandler, DCSGATEDATA);
						CreateBeamHeader(beamHandler, tn, info, targetTN, false, true);


						for (auto& client : _clients)
						{
							client->write(buffer, handler.GetByteOffset());
							client->write(beamBuffer, beamHandler.GetByteOffset());
							SendAllCategoryData(client, tn, info.record);
						}

						//ORIGINAL//
						CreateEvent(eventHandler, m_beamId, DCSSTATE_ON, "CREATE TEST", DCSEVENT_STATE);
						if (eventHandler.GetByteOffset())
						{
							for (auto& client : _clients)
							{
								client->write(eventBuffer, eventHandler.GetByteOffset());
							}
						}
						//TargetID
						//if (tn == 135169) //AAOO1
						//{
						//	targetTN = 152065; //BB001
						//	DLPSWriteChar(beamHandler, DCSGATEDATA);
						//	CreateBeamHeader(beamHandler, tn, info, targetTN,DCSBEAM_STEADY, true, true);
						//	CreateEvent(eventHandler, m_beamId, targetTN, "CREATE TEST", DCSEVENT_TARGETID);
						//	if (targetTN == 152065)
						//	{
						//		targetTN = 168961; //CC001
						//	}
						//	if (eventHandler.GetByteOffset())
						//	{
						//		for (auto& client : _clients)
						//		{
						//			client->write(beamBuffer, beamHandler.GetByteOffset());
						//			client->write(eventBuffer, eventHandler.GetByteOffset());
						//		}
						//	}
						//}
						//else
						//{
						//	targetTN = -1; 
						//	DLPSWriteChar(beamHandler, DCSGATEDATA);
						//	CreateBeamHeader(beamHandler, tn, info, targetTN, DCSBEAM_LINEAR, false, true);
						//	CreateEvent(eventHandler, m_beamId, targetTN, "CREATE TEST", DCSEVENT_COLOR);
						//	if (eventHandler.GetByteOffset())
						//	{
						//		for (auto& client : _clients)
						//		{
						//			client->write(beamBuffer, beamHandler.GetByteOffset());
						//			client->write(eventBuffer, eventHandler.GetByteOffset());
						//		}
						//	}
						//}
					}
				}
				else
				{
					emit sigChangeStatus(SIMDISStatusUpdateTrack, QString("%1").arg(J_REFERENCE_TN_TO_TEXT(tn)));

					//if (_clients.size())
					{
						WriteCharacters(handler, DCS_SERVERIDSZ, _serverID);
						CreatePlatformData(handler, tn, iter->second, latitude, longitude, altitude, true);
						QUdpSocket().writeDatagram(buffer, handler.GetByteOffset(), _simdisAddr, _simdisPort);

						//QUdpSocket().writeDatagram(beamBuffer, beamHandler.GetBitOffset(), _simdisAddr, _simdisPort);
					}
				}
			}
			else if (id == MHIMessageIDSpaceTrack)
			{
				double positionX = atof(GetValue(record, "X POSITION IN WGS-84").GetData());
				double positionY = atof(GetValue(record, "Y POSITION IN WGS-84").GetData());
				double positionZ = atof(GetValue(record, "Z POSITION IN WGS-84").GetData());

				double latitude = 0.0;
				double longitude = 0.0;
				double altitude = 0.0;
				ConvertWGS84XYZToLatitudeLongitudeHeight(positionX, positionY, positionZ, latitude, longitude, altitude);
				//altitude = ConvertFeetToMeters(altitude);
				altitude = altitude / 1000.0;

				unsigned long long identity = 0;
				//record.GetFieldCode("IDENTITY", identity);

				bool first = false;
				map<unsigned long long, TrackInfo>::iterator iter = _tracks.find(tn);
				if (iter == _tracks.end())
				{
					first = true;
				}

				if (first)
				{
					emit sigChangeStatus(SIMDISStatusNewTrack, QString("%1").arg(J_REFERENCE_TN_TO_TEXT(tn)));

					TrackInfo info;
					//info.identity = identity;
					info.latitude = latitude;
					info.longitude = longitude;
					info.altitude = altitude;
					info.record.SetBuffer(record.GetBuffer(), record.GetBufferSize());

					//int index = rand() % CNT_MODEL_NAME;
					//info.iconname = strMissiles[index];
					info.iconname = strMissiles[0];

					_tracks[tn] = info;

					if (_clients.size())
					{
						DLPSWriteChar(handler, DCSGATEDATA);

						CreatePlatformHeader(handler, tn, info, (char*)J_REFERENCE_TN_TO_TEXT(tn));
						for (auto& client : _clients)
						{
							client->write(buffer, handler.GetByteOffset());
						}
					}
				}
				else
				{
					emit sigChangeStatus(SIMDISStatusUpdateTrack, QString("%1").arg(J_REFERENCE_TN_TO_TEXT(tn)));

					if (_clients.size())
					{
						WriteCharacters(handler, DCS_SERVERIDSZ, _serverID);
						CreatePlatformData(handler, tn, iter->second, latitude, longitude, altitude, true);
						QUdpSocket().writeDatagram(buffer, handler.GetByteOffset(), _simdisAddr, _simdisPort);
					}
				}
			}
			else if (id == MHIMessageIDTrackManagement)
			{
				emit sigChangeStatus(SIMDISStatusDropTrack, QString("%1").arg(J_REFERENCE_TN_TO_TEXT(tn)));

				unsigned long long action = 0;
				if (record.GetFieldCode("ACTION, TRACK MANAGEMENT", action) && action == TRACK_MANAGEMENT_ACTION_DROP)
				{
					CreateEvent(eventHandler, tn, DCSSTATE_EXPIRE, "DROP TRACK", DCSEVENT_STATE);
					if (eventHandler.GetByteOffset())
					{
						for (auto& client : _clients)
						{
							client->write(eventBuffer, eventHandler.GetByteOffset());
						}
					}

					//handler.SetOffset(192);
					//DLPSWriteChar(handler, DCSSTATE_EXPIRE);
					//m_container->Update(handler, tn, true);

					map<unsigned long long, TrackInfo>::iterator iter = _tracks.find(tn);
					if (iter != _tracks.end())
					{
						_tracks.erase(iter);
					}
				}
			}
			else if (id == MHIMessageIDEngagementStatus)
			{
				if (m_timerSendMissile.isActive())
				{
					return;
				}

				if (!record.GetFieldCode("TRACK NUMBER, REFERENCE", tn))
				{
					return;
				}

				map<unsigned long long, TrackInfo>::iterator iter = _tracks.find(tn);
				if (iter == _tracks.end())
				{
					return;
				}

				double tmp;
				double curX;
				double curY;
				double curZ;
				GetTargetLatLong(_tracks[tn].latitude, _tracks[tn].longitude, 90, _tracks[tn].positionX, tmp, curY);
				GetTargetLatLong(_tracks[tn].latitude, _tracks[tn].longitude, 0, _tracks[tn].positionY, curX, tmp);
				curZ = _tracks[tn].positionZ + _tracks[tn].altitude;

				_StartPos.setX(curX);
				_StartPos.setY(curY);
				_StartPos.setZ(curZ);

				unsigned long long target;
				if (!record.GetFieldCode("TRACK NUMBER, TARGET", target))
				{
					return;
				}
				iter = _tracks.find(target);
				if (iter == _tracks.end())
				{
					return;
				}

				m_objectTN = target;
				GetTargetLatLong(_tracks[target].latitude, _tracks[target].longitude, 90, _tracks[target].positionX, tmp, curY);
				GetTargetLatLong(_tracks[target].latitude, _tracks[target].longitude, 0, _tracks[target].positionY, curX, tmp);
				curZ = _tracks[target].positionZ + _tracks[target].altitude;

				_EndPos.setX(curX);
				_EndPos.setY(curY);
				_EndPos.setZ(curZ);

				m_timerSendMissile.start(60);	// DrawMissile();
			}
		}
	}
}

void SocketSIMDIS::WriteBase(DLPSBufferHandler& handler, DCSDataType_t type, unsigned long long id)
{
	DLPSWriteShort(handler, type);		// type
	DLPSWriteShort(handler, DCS_VERSION);			// version
	DLPSWriteLongLong(handler, id);	// id
}
void SocketSIMDIS::WriteReferenceFrame(DLPSBufferHandler& handler, double latitude, double longitude, double altitude)
{
	DLPSWriteChar(handler, DCSCOORD_ENU);	//coordsystem_
	WriteDouble(handler, ConvertToRadians(latitude));	// latorigin_
	WriteDouble(handler, ConvertToRadians(longitude));	// lonorigin_
	WriteDouble(handler, altitude);	// altorigin_

	DLPSWriteChar(handler, DCSVERTDATUM_WGS84);		// verticalDatum_
	WriteDouble(handler, 0.0);						// verticalOffset_
	DLPSWriteChar(handler, DCSMAGVAR_TRUE);			// magneticVariance_
	WriteDouble(handler, 0.0);						// magneticOffset_
	WriteDouble(handler, 0.0);						// referenceTimeECI_
	WriteDouble(handler, 0.0);						// tangentOffsetX_
	WriteDouble(handler, 0.0);						// tangentOffsetY_
	WriteDouble(handler, 0.0);						// tangentAngle_
}


void SocketSIMDIS::CreatePlatformHeader(DLPSBufferHandler& handler, unsigned long long tn, TrackInfo info, const char* callsign)
{
	WriteBase(handler, DCSPLATFORMHEADER, tn);

	// DCSPlatformHeader (183)
	WriteCharacters(handler, 64, info.iconname.GetData());		//iconname_
	WriteCharacters(handler, 64, callsign);//J_REFERENCE_TN_TO_TEXT(tn));	//callsign_

	//dims_
	WriteDouble(handler, 0.0);	// DCSPoint.x
	WriteDouble(handler, 0.0);	// DCSPoint.y
	WriteDouble(handler, 0.0);	// DCSPoint.z

	//offset_
	WriteDouble(handler, 0.0);	// DCSPoint.x
	WriteDouble(handler, 0.0);	// DCSPoint.y
	WriteDouble(handler, 0.0);	// DCSPoint.z

	DLPSWriteInteger(handler, 0);	// color_

	DLPSWriteChar(handler, 0);	// state_
	DLPSWriteChar(handler, DCSSTATUS_LIVE);	// status_
	DLPSWriteChar(handler, true);	// interpolate_

	// DCSCoordReferenceFrame (75)
	WriteReferenceFrame(handler, info.latitude, info.longitude, info.altitude);

	// orientationoffset_ (24)
	WriteDouble(handler, 0.0); // DCSPoint.x
	WriteDouble(handler, 0.0); // DCSPoint.y
	WriteDouble(handler, 0.0); // DCSPoint.z

	CreatePlatformData(handler, tn, info);
}
void SocketSIMDIS::CreateCategoryData(DLPSBufferHandler& handler, unsigned long long tn, QString name, QString value)
{
	DLPSWriteChar(handler, DCSREQUEST_HEADER);
	WriteBase(handler, DCSCATEGORYDATA, tn);
	WriteDouble(handler, yeartime());	// time_
	WriteCharacters(handler, 64, name.toLocal8Bit().data()); //name_
	WriteCharacters(handler, 64, value.toLocal8Bit().data()); //value_
}
void SocketSIMDIS::SendAllCategoryData(QTcpSocket* client, unsigned long long tn, TDBRecord record)
{
	// TO DO : 사용 하려면 DLPS INITIALZIE 시 LKMessageMap 필요
	return;
	if (client)
	{
		TDBField field;
		int total = record.GetTotalFields();
		for (int i = 0; i < total; i++)
		{
			if (record.GetField(i, field) && field.GetName()[0] != '!' && field.IsSet())
			{
				char buffer[MAX_DATAGRAM_SIZE];
				DLPSBufferHandler handler(buffer);
				CreateCategoryData(handler, tn, field.GetName(), GetValue(record, field.GetName()).GetData());
			}
		}
	}
}

void SocketSIMDIS::CreatePlatformData(DLPSBufferHandler& handler, unsigned long long tn, TrackInfo& orgInfo, double latitude, double longitude, double altitude, bool update)
{
	WriteBase(handler, DCSPLATFORMDATA, tn);

	if (update)
	{
		double preLat;
		double preLng;
		double preAlt;

		double tmp;

		GetTargetLatLong(orgInfo.latitude, orgInfo.longitude, 90, orgInfo.positionX, tmp, preLng);
		GetTargetLatLong(orgInfo.latitude, orgInfo.longitude, 0, orgInfo.positionY, preLat, tmp);
		preAlt = orgInfo.positionZ + orgInfo.altitude;

		double disX = GetDistance(preLat, preLng, preLat, longitude);
		double disY = GetDistance(preLat, preLng, latitude, preLng);
		double disZ = altitude - preAlt;


		//orientationOffset
		orgInfo.orientationX = GetBearing(preLat, preLng, latitude, longitude) * DEG2RAD;	// course
		orgInfo.orientationY = asin(disZ / sqrt(disX*disX + disY * disY + disZ*disZ));		//
		//orgInfo.orientationZ = 0.0;

		// position
		orgInfo.positionX = GetDistance(orgInfo.latitude, orgInfo.longitude, orgInfo.latitude, longitude);
		if (longitude < orgInfo.longitude)
		{
			orgInfo.positionX *= -1;
		}

		orgInfo.positionY = GetDistance(orgInfo.latitude, orgInfo.longitude, latitude, orgInfo.longitude);
		if (latitude < orgInfo.latitude)
		{
			orgInfo.positionY *= -1;
		}

		orgInfo.positionZ = altitude - orgInfo.altitude;

	}

	// position_ (24)
	WriteDouble(handler, orgInfo.positionX);// DCSPoint.x
	WriteDouble(handler, orgInfo.positionY);// DCSPoint.y
	WriteDouble(handler, orgInfo.positionZ);// DCSPoint.z

	// orientation_ (24)
	WriteDouble(handler, orgInfo.orientationX);	// DCSPoint.x
	WriteDouble(handler, orgInfo.orientationY);	// DCSPoint.y
	WriteDouble(handler, 0.0);	// DCSPoint.z

	// velocity_ (24)
	WriteDouble(handler, m_vx);	// DCSPoint.x
	WriteDouble(handler, m_vy);	// DCSPoint.y
	WriteDouble(handler, m_vz);	// DCSPoint.z

	// accel_ (24)
	WriteDouble(handler, 0.0);	// DCSPoint.x
	WriteDouble(handler, 0.0);	// DCSPoint.y
	WriteDouble(handler, 0.0);	// DCSPoint.z

	WriteDouble(handler, yeartime());	// time_
}

void SocketSIMDIS::WriteDouble(DLPSBufferHandler& handler, double value)
{
	char* buffer = (char*)handler.GetBuffer();
	for (int i = 7; i >= 0; i--)
	{
		memcpy(buffer + handler.GetByteOffset() + 7 - i, (char*)(&value) + i, 1);
	}
	handler.Skip(8);
}
void SocketSIMDIS::WriteCharacters(DLPSBufferHandler& handler, int length, const char* characters)
{
	char* buffer = (char*)handler.GetBuffer();
	DLPSFillBytes(handler, 0, length);
	int strLen = strlen(characters);
	if (strLen > length)
	{
		strLen = length;
	}
	memcpy(buffer + handler.GetByteOffset() - length, characters, strLen);
}

void SocketSIMDIS::CreateBeamHeader(DLPSBufferHandler& handler, unsigned long long tn, TrackInfo info, unsigned long long targetTN, int beamType, bool bTargetTN, bool bMissile)
{
	WriteBase(handler, DCSBEAMHEADER, m_beamId);

	// DCSBeamHeader (84)
	DLPSString strCall_ = J_REFERENCE_TN_TO_TEXT(tn);
	strCall_.Append("_Beam");
	WriteCharacters(handler, 64, strCall_);	//callsign_

	//// //TO DO// ///
	//offset_ from the plaform 
	WriteDouble(handler, 0.0);	//missileoffset_ DCSPoint.x
	WriteDouble(handler, 0.0);	//missileoffset_ DCSPoint.y
	WriteDouble(handler, 0.0);	//missileoffset_ DCSPoint.z

	WriteDouble(handler, 0.0);	//azimoffset_
	WriteDouble(handler, 0.0);	//elevoffset_

	DLPSWriteChar(handler, bMissile);	//BOOL missileoffset_

	WriteDouble(handler, 3 * DEG2RAD);	//hbw_
	WriteDouble(handler, 3 * DEG2RAD);	//vbw_	

	DLPSWriteLongLong(handler, tn); //hostid_

	DLPSWriteLongLong(handler, targetTN); //targetid_	case: non-target{ targetTN = -1};

	DLPSWriteInteger(handler, 0xFFFFFFFF);	// color_

	DLPSWriteChar(handler, DCSSTATE_ON);	// state_
	DLPSWriteChar(handler, beamType);	// beamtype_  :: DCSBEAM_LINEAR | DCSBEAM_STEADY
	DLPSWriteChar(handler, true);	// interpolate_

	CreateBeamData(handler, tn);
}

// fixes angles, in radians, between 0 and two PI
inline double ANGFIX(double x)
{
	return ((x = fmod(x, (M_PI*2.0)))<0.0) ? (x + (M_PI*2.0)) : x;
}

void SocketSIMDIS::CreateBeamData(DLPSBufferHandler& handler, unsigned long long tn)
{
	WriteBase(handler, DCSBEAMDATA, tn);

	//size(32)
	//timeT += 1.0;
	WriteDouble(handler, yeartime());			// time_
	double azim = ANGFIX(yeartime() * DEG2RAD);
	WriteDouble(handler, azim);				// azim_	
	WriteDouble(handler, 15.0 * DEG2RAD);	// elev_
	WriteDouble(handler, 300.0);			// length_
}

void SocketSIMDIS::CreateEvent(DLPSBufferHandler& handler, unsigned long long id, unsigned long long data, char* charData, __int8 eventType)
{
	//WriteBase(handler, DCSEVENT, id);

	DLPSWriteChar(handler, DCSREQUEST_EVENT);
	WriteBase(handler, DCSEVENT, id);

	WriteDouble(handler, yeartime());			// time_
	DLPSWriteLongLong(handler, data);	// data_
	WriteCharacters(handler, 128, charData);	//chardata_
	DLPSWriteChar(handler, eventType); //type
}

void SocketSIMDIS::DrawMissile()
{
	double endTime; //도착지점 도달 시간
	double maxHeight = 100000; //최대 높이
	double height; //altitude? // 최대높이의 Z - 시작높이의 Z
	double endHeight; //도착지점의 높이 Z - 시작지점의 높이 Z
	double maxTime = 5.0f; //최대 높 이까지 가는 시간.

	endHeight = _EndPos.z() - _StartPos.z();
	height = maxHeight - _StartPos.z();

	m_g = 2 * height / (maxTime * maxTime);
	m_vz = sqrtf(2 * m_g * height);

	double a = m_g;
	double b = (-2) * m_vz;
	double c = 2 * endHeight;

	endTime = (-b + sqrtf(b*b - 4 * a*c)) / (2 * a);

	m_vx = -(_StartPos.x() - _EndPos.x()) / endTime;
	m_vy = -(_StartPos.y() - _EndPos.y()) / endTime;
	
	QVector3D CurrentPos;

	//dTime += 0.01 * m_timerSendMissile.interval();
	dTime += 0.01;
	CurrentPos.setX(_StartPos.x() + m_vx * dTime);
	CurrentPos.setY(_StartPos.y() + m_vy * dTime);
	CurrentPos.setZ(_StartPos.z() + (m_vz * dTime) - (0.5f * m_g * dTime * dTime));

	//SendMissileData(CurrentPos.x(), CurrentPos.y(), CurrentPos.z(), m_vx, m_vy, m_vz);

	// pow : 양수
	double intervalX = sqrt(pow(CurrentPos.x() - _EndPos.x(), 2.0));
	double intervalY = sqrt(pow(CurrentPos.y() - _EndPos.y(), 2.0));
	if (intervalX < 0.05 && intervalY < 0.05 && CurrentPos.z() < _EndPos.z())
	{
		m_timerSendMissile.stop();
		dTime = 0.0;

		//Drop Target 
		TDBRecord record("TRACK MANAGEMENT");
		//// TO DO : 기타 필드 값들 채우기
		//record.SetFieldCode("ACTION, TRACK MANAGEMENT", TRACK_MANAGEMENT_ACTION_DROP);
		//record.SetFieldCode("TRACK NUMBER, REFERENCE", target);
		//SendTrackMessage(MHIMessageIDTrackManagement, record.GetBuffer(), record.GetBufferSize());
		//record.SetFieldCode("TRACK NUMBER, REFERENCE", missileTN);
		//SendTrackMessage(MHIMessageIDTrackManagement, record.GetBuffer(), record.GetBufferSize());
	}

	SendMissileData(CurrentPos.x(), CurrentPos.y(), CurrentPos.z(), m_vx, m_vy, m_vz);

}

void SocketSIMDIS::GetTargetLatLong(double sourceLat, double sourceLong,
	double bearing, double distance,
	double &destLat, double &destLong)
{
	double a = HTGIS_WGS84_EARTH_MAJOR_AXIS, b = HTGIS_WGS84_EARTH_MINOR_AXIS, f = HTGIS_WGS84_EARTH_FLATNESS; // WGS-84 ellipsoid
	double s = distance;
	double alpha1 = bearing * HTGIS_DECIMAL_DEGREES_TO_RADIAN;
	double sin_alpha1 = sin(alpha1), cos_alpha1 = cos(alpha1);

	double tan_u1 = (1.0 - f) * tan(sourceLat * HTGIS_DECIMAL_DEGREES_TO_RADIAN);
	double cos_u1 = 1.0 / sqrt(1.0 + tan_u1 * tan_u1), sin_u1 = tan_u1 * cos_u1;
	double sigma1 = atan2(tan_u1, cos_alpha1);
	double sin_alpha = cos_u1 * sin_alpha1;
	double cos_sq_alpha = 1.0 - sin_alpha * sin_alpha;
	double u_sq = cos_sq_alpha * (a * a - b * b) / (b * b);
	double A = 1 + u_sq / 16384.0 * (4096.0 + u_sq * (-768.0 + u_sq * (320.0 - 175.0 * u_sq)));
	double B = u_sq / 1024.0 * (256.0 + u_sq * (-128.0 + u_sq * (74.0 - 47.0 * u_sq)));

	double sin_sigma = 0.0, cos_sigma = 1.0;
	double cos_2_sigma_m = cos(2.0 * sigma1);
	double delta_sigma;

	double sigma = s / (b * A), sigma_p = HTGIS_TWO_PI;
	while (fabs(sigma - sigma_p) > HTGIS_DOUBLE_PRECISION_EPSILON)
	{
		cos_2_sigma_m = cos(2.0 * sigma1 + sigma);
		sin_sigma = sin(sigma);
		cos_sigma = cos(sigma);
		delta_sigma = B * sin_sigma * (cos_2_sigma_m + B / 4.0 * (cos_sigma * (-1.0 + 2.0 * cos_2_sigma_m * cos_2_sigma_m) - B / 5.0 * cos_2_sigma_m * (-3.0 + 4.0 * sin_sigma * sin_sigma) * (-3.0 + 4.0 * cos_2_sigma_m * cos_2_sigma_m)));
		sigma_p = sigma;
		sigma = s / (b * A) + delta_sigma;
	}

	double tmp = sin_u1 * sin_sigma - cos_u1 * cos_sigma * cos_alpha1;
	double lat = atan2(sin_u1 * cos_sigma + cos_u1 * sin_sigma * cos_alpha1,
		(1.0 - f) * sqrt(sin_alpha * sin_alpha + tmp * tmp));
	double lambda = atan2(sin_sigma * sin_alpha1, cos_u1 * cos_sigma - sin_u1 * sin_sigma * cos_alpha1);
	double C = f / 16.0 * cos_sq_alpha * (4.0 + f * (4.0 - 3.0 * cos_sq_alpha));
	double L = lambda - (1.0 - C) * f * sin_alpha * (sigma + C * sin_sigma * (cos_2_sigma_m + C * cos_sigma *  (-1.0 + 2.0 * cos_2_sigma_m * cos_2_sigma_m)));

	//	double rev_az = atan2(sin_alpha, -tmp);

	destLat = lat * HTGIS_RADIAN_TO_DECIMAL_DEGREES;

	sourceLong += L * HTGIS_RADIAN_TO_DECIMAL_DEGREES;
	if (sourceLong > 180.0)
		sourceLong -= 360.0;
	destLong = sourceLong;
}

//미사일에 격추당했을때 platform지우기
void SocketSIMDIS::SendDropMessage(unsigned long long target, unsigned long long missileTN)
{
	//Drop Target 
	//TDBRecord record("TRACK MANAGEMENT");
	// TO DO : 기타 필드 값들 채우기
	//record.SetFieldCode("ACTION, TRACK MANAGEMENT", TRACK_MANAGEMENT_ACTION_DROP);
	//record.SetFieldCode("TRACK NUMBER, REFERENCE", target);
	//SendTrackMessage(MHIMessageIDTrackManagement, record.GetBuffer(), record.GetBufferSize());
	//record.SetFieldCode("TRACK NUMBER, REFERENCE", missileTN);
	//SendTrackMessage(MHIMessageIDTrackManagement, record.GetBuffer(), record.GetBufferSize());
}