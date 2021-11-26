// C
#include <stdio.h>

// Qt
#include <qapplication.h>
#include <qpixmap.h>
#include <qwidget.h>

// Defines
#define SCR_WIDTH  240
#define SCR_HEIGHT 320

int main(int argc, char *argv[]) {
	if (argc >= 2 && argc <= 3) {
		QApplication app(argc, argv);
		QWidget fullScreenWidget(0);
		fullScreenWidget.setFocusPolicy(QWidget::NoFocus);
		fullScreenWidget.setBackgroundMode(QWidget::NoBackground);
		fullScreenWidget.setFixedSize(SCR_WIDTH, SCR_HEIGHT);
		fullScreenWidget.show();
		QPixmap fullScreenPixmap(SCR_WIDTH, SCR_HEIGHT, -1, QPixmap::BestOptim);
		bitBlt(&fullScreenPixmap, 0, 0, &fullScreenWidget, 0, 0, SCR_WIDTH, SCR_HEIGHT, Qt::CopyROP, true);
		if (argc == 2)
			fullScreenPixmap.save(argv[1], QStringList::split(".", argv[1])[1].upper());
		else
			fullScreenPixmap.save(argv[1], QStringList::split(".", argv[1])[1].upper(), QString(argv[2]).toInt());
	} else
		fprintf(
			stderr,
			"Usage:\n"
			"\tzgrab screenshot.<format> <quality 0-100>\n\n"
			"Example:\n"
			"\tzgrab screenshot.png\n"
			"\tzgrab screenshot.bmp\n"
			"\tzgrab screenshot.jpeg 100\n"
			"\tzgrab screenshot.jpeg 25\n"
		);
	return 0;
}
