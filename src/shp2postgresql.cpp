#include "../include/shp2postgresql.h"
#include "../res/ui_shp2postgresql.h"

ShapefileConversion::Shp2Postgresql::Shp2Postgresql(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Shp2Postgresql)
{
    ui->setupUi(this);
}

ShapefileConversion::Shp2Postgresql::~Shp2Postgresql()
{
    delete ui;
}

