// C
#include <stdio.h>

// Qt
#include <qapplication.h>
#include <qpixmap.h>
#include <qwidget.h>

int main(int argc, char *argv[]) {
	if (argc >= 2 && argc <= 3) {
		QApplication app(argc, argv);
		QPixmap::setDefaultOptimization(QPixmap::BestOptim);
		QPixmap fullScreenPixmap = QPixmap::grabWindow(QApplication::desktop()->winId());
		if (argc == 2)
			fullScreenPixmap.save(argv[1], QStringList::split(".", argv[1])[1].upper());
		else
			fullScreenPixmap.save(argv[1], QStringList::split(".", argv[1])[1].upper(), QString(argv[2]).toInt());
	} else
		fprintf(
			stderr,
			"Usage:\n"
			"\tdgrab screenshot.<format> <quality 0-100>\n\n"
			"Example:\n"
			"\tdgrab screenshot.png\n"
			"\tdgrab screenshot.bmp\n"
			"\tdgrab screenshot.jpeg 100\n"
			"\tdgrab screenshot.jpeg 25\n"
		);
	return 0;
}
