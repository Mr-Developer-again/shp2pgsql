#include "../include/shp2postgresql.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ShapefileConversion::Shp2Postgresql w;
    w.show();
    return a.exec();
}
