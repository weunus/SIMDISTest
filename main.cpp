#include "DlgMainWindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	DlgMainWindow w;
	w.show();
	return a.exec();
}
