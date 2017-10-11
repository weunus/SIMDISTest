#ifndef THREAD_INITIALIZE_DLPS_H
#define THREAD_INITIALIZE_DLPS_H

#include <QThread>

class InitDLPS : public QThread
{
	Q_OBJECT

private:
	void run();

signals:
	void sigInitialize(int, QString log = "");
};

#endif //THREAD_INITIALIZE_DLPS_H