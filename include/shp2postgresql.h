#ifndef SHAPEFILECONVERSION_SHP2POSTGRESQL_H
#define SHAPEFILECONVERSION_SHP2POSTGRESQL_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Shp2Postgresql; }
QT_END_NAMESPACE

namespace ShapefileConversion
{

    class Shp2Postgresql : public QMainWindow
    {
        Q_OBJECT

    public:
        Shp2Postgresql(QWidget *parent = nullptr);
        ~Shp2Postgresql();

    private:
        Ui::Shp2Postgresql *ui;
    };

}
#endif // SHAPEFILECONVERSION_SHP2POSTGRESQL_H
