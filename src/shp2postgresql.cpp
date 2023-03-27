#include "../include/shp2postgresql.h"

#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QFile>

#include <stdexcept>
#include <regex>
#include <stdio.h>

ShapefileConversion::Shp2Postgresql::Shp2Postgresql(QWidget *parent)
    : QMainWindow(parent),
      _ui(std::make_unique<Ui::Shp2Postgresql>())
{
    this->_ui->setupUi(this);
    this->setFixedSize(this->geometry().width(), this->geometry().height());


    QObject::connect(this->_ui->shapefilePathBrowseButton, SIGNAL(clicked()), this, SLOT(slot_shpPathBrowseButton()));
    QObject::connect(this->_ui->cancelButton, SIGNAL(clicked()), this, SLOT(slot_cancelButtonClicked()));
    QObject::connect(this->_ui->importButton, SIGNAL(clicked()), this, SLOT(slot_importButtonClicked()));
}

bool ShapefileConversion::Shp2Postgresql::allSectionsFilled() const
{
    if (this->_ui->shapefilePathLineEdit->text().isEmpty())
        throw std::runtime_error("You Should Fill Shapefile Path Section");

    else if (this->_ui->dbNameLineEdit->text().isEmpty())
        throw std::runtime_error("You Should Fill Database Name Section");

    else if (this->_ui->tableNameLineEdit->text().isEmpty())
        throw std::runtime_error("You Should Fill Table Name Section");

    else if (this->_ui->hostIpAddrLineEdit->text().isEmpty())
        throw std::runtime_error("You Should Fill Host IP Address Section");

    else if (this->_ui->usernameLineEdit->text().isEmpty())
        throw std::runtime_error("You Should Fill Username Section");

    return true;
}

bool ShapefileConversion::Shp2Postgresql::userInputsAreValid() const
{
    // Validating shapefile path (the input file exists or not)
    QFile file(this->_ui->shapefilePathLineEdit->text().trimmed());
    if (!file.exists())
        throw std::runtime_error("The Input Shapefile Path Doesn't Exists");

    // validating input database name
    std::regex pgsqlValidDBNamePattern("^[A-Za-z][A-Za-z0-9_]{0,62}$");
    if (!std::regex_match(this->_ui->dbNameLineEdit->text().trimmed().toStdString(), pgsqlValidDBNamePattern))
        throw std::runtime_error("The Input Database Name Is Not A Valid DB Name in PostgreSQL");

    // validating input table name
    std::regex pgsqlValidTableNamePattern("^[A-Za-z][A-Za-z0-9_]{0,62}$");
    if (!std::regex_match(this->_ui->tableNameLineEdit->text().trimmed().toStdString(), pgsqlValidTableNamePattern))
        throw std::runtime_error("The Input Table Name Is Not A Valid Table Name in PostgreSQL");

    // validating ip address syntax
    // Regex to validate IP address format
    std::regex validIpPattern("^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$");
    if (!std::regex_match(this->_ui->hostIpAddrLineEdit->text().trimmed().toStdString(), validIpPattern))
        throw std::runtime_error("Invalid Syntax For Host IP Address");


    // Validating input username
    char* osName;

#if defined(__linux__)
    // linux username pattern
    std::regex validUsernamePattern("^[a-z_][a-z0-9_-]{0,31}$");
    osName = "Linux";

    // windows username pattern
#elif defined(_WIN32) || defined(_WIN64)
    std::regex validUsernamePattern("^[a-zA-Z][a-zA-Z0-9._-]{0,19}$");
    osName = "Windows";

    // osx (macOS) username pattern
#elif defined(__APPLE__)
    std::regex validUsernamePattern("^[a-zA-Z][a-zA-Z0-9._-]{0,30}$");
    osName = "MacOS";
#endif

    if (!std::regex_match(this->_ui->usernameLineEdit->text().trimmed().toStdString(), validUsernamePattern))
    {
        char* message;
        sprintf(message, "The input username is not a valid username for %s system", osName);
            throw std::runtime_error(message);
    }
}

void ShapefileConversion::Shp2Postgresql::slot_shpPathBrowseButton()
{
    QString filePath = QFileDialog::getOpenFileName(
                nullptr,
                QString("Select Shapefile"),
                QDir::homePath(),
                QString("Shp (*.shp)")
    );

    if (!filePath.isEmpty())
        this->_ui->shapefilePathLineEdit->setText(filePath);
}

void ShapefileConversion::Shp2Postgresql::slot_cancelButtonClicked()
{
    this->close();
}

void ShapefileConversion::Shp2Postgresql::slot_importButtonClicked()
{
    try
    {
        if (this->allSectionsFilled() && this->userInputsAreValid())
            ;
    }
    catch(std::runtime_error const& ex)
    {
        QMessageBox::critical(nullptr, "Runtime Error", ex.what());
    }
    catch(std::exception const& ex)
    {
        QMessageBox::critical(nullptr, "Exception Occured!", ex.what());
    }
    catch(...)
    {
        QMessageBox::critical(nullptr, "Unknown Error!", "Something Went wrong");
    }
}
