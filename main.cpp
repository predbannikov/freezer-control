#include <QApplication>
#include <QDebug>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
        qDebug() << ">>> main() start";
    QApplication a(argc, argv);
            qDebug() << ">>> QApplication created";
    MainWindow w;
                qDebug() << ">>> MainWindow constructed";
    w.show();

    return a.exec();
}
