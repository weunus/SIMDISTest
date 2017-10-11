#ifndef SOCKET_DLPS_H
#define SOCKET_DLPS_H 1

#include <QUdpSocket>
#include <QString>
#include <QHostAddress>
#include "dlps/DLPSMHIParser.h"

class SocketDLPS : public QObject, public DLPSMHIParser
{
	Q_OBJECT

public:
	SocketDLPS();

	bool Join(QString addr, int port);
	bool Leave();

private slots:
	void OnReadyRead();

signals:
	void sigReceiveMessage(const ICDMessageHeader *header, const MHIMessageSubheader *subheader,
						   const char *log,
						   const void *record, unsigned int length);

private:
	void OnDecodeMessage(const ICDMessageHeader *header, const MHIMessageSubheader *subheader,
		const char *log,
		const void *record, unsigned int length);
	void OnDecodeError(const char *reason);

private:
	QHostAddress _address;
	QUdpSocket	 _socket;
	int			 _port;
};

#endif //SOCKET_DLPS_H