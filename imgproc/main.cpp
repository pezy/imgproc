#include "mainwindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setOrganizationName("pezy");
	a.setApplicationName("imgproc");
	MainWindow w;
	w.showMaximized();
	return a.exec();
}
