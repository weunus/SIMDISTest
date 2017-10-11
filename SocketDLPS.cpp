#include "SocketDLPS.h"
#include <QApplication>

SocketDLPS::SocketDLPS()
{
	moveToThread(QApplication::instance()->thread());
}

bool SocketDLPS::Join(QString addr, int port)
{
	bool success = false;
	_address = addr;
	_port = port;
	if (_socket.bind(QHostAddress::AnyIPv4, port, QUdpSocket::ReuseAddressHint) &&
		_socket.joinMulticastGroup(_address))
	{
		success = true;
		connect(&_socket, SIGNAL(readyRead()), this, SLOT(OnReadyRead()));
	}
	return success;
}
bool SocketDLPS::Leave()
{
	return _socket.leaveMulticastGroup(_address);
}

void SocketDLPS::OnReadyRead()
{
	qint64 recvSize;
	char *message;
	while (_socket.hasPendingDatagrams())
	{
		recvSize = _socket.pendingDatagramSize();
		message = new char[recvSize];

		if ((_socket.readDatagram(message, recvSize) != -1))
		{
			if (!DLPSMHIParser::Decode(message, recvSize))
			{
				char* reason;
				OnDecodeError(reason);
				return;
			}
		}
	}
}

void SocketDLPS::OnDecodeMessage(const ICDMessageHeader *header, const MHIMessageSubheader *subheader,
								 const char *log,
								 const void *record, unsigned int length)
{
	emit sigReceiveMessage(header, subheader, log, record, length);
}
void SocketDLPS::OnDecodeError(const char *reason)
{

}