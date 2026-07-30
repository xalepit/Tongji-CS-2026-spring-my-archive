#include "ui/MainWindow.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("DNA Search Engine"));
    application.setOrganizationName(QStringLiteral("Tongji University"));
    application.setStyle(QStringLiteral("Fusion"));
    application.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));

    MainWindow window;
    window.show();
    return QApplication::exec();
}
