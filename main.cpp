#include "facerecognitionstatistics.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	facerecognitionstatistics w;
	w.show();
	return a.exec();
}
