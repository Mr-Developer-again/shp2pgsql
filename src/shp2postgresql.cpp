#include "../include/shp2postgresql.h"

#include <QFileDialog>
#include <QString>
#include <QMessageBox>

#include <iostream>
#include <stdexcept>
#include <regex>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <functional>
#include <fstream>
#include <chrono>
#include <thread>

ShapefileConversion::Shp2Postgresql::Shp2Postgresql(QWidget *parent)
    : QMainWindow(parent),
      _ui(std::make_unique<Ui::Shp2Postgresql>())
{
    this->_ui->setupUi(this);
    this->setFixedSize(this->geometry().width(), this->geometry().height());

    this->_ui->sridSpinBox->setMaximum(std::numeric_limits<int>::max());

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
    std::string osName;

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
        sprintf(message, "The input username is not a valid username for %s system", osName.c_str());
        throw std::runtime_error(message);
    }

    return true;
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
        if (this->allSectionsFilled() and this->userInputsAreValid())
        {
            std::string shapefilePath = this->_ui->shapefilePathLineEdit->text().toStdString();
            std::string srid = this->_ui->sridSpinBox->text().toStdString();
            std::string dbName = this->_ui->dbNameLineEdit->text().toStdString();
            std::string tableName = this->_ui->tableNameLineEdit->text().toStdString();
            std::string hostAddr = this->_ui->hostIpAddrLineEdit->text().toStdString();
            std::string username = this->_ui->usernameLineEdit->text().toStdString();

            // changing stderr and stdout
            std::unique_ptr<FILE, std::function<void(FILE*)>> fd_stderr(freopen("./.fd_stderr", "w", stderr), [](FILE* fd) -> void {
                fclose(fd);
            });

            std::unique_ptr<FILE, std::function<void(FILE*)>> fd_stdout(freopen("./.fd_stdout", "w", stdout), [](FILE* fd) -> void {
                fclose(fd);
            });

            if (NULL == fd_stderr || NULL == fd_stdout)
            {
                fputs("couldn't change stdout/stderr stream", stderr);
                exit(EXIT_FAILURE);
            }

            std::stringstream command;
            command << "shp2pgsql -s " << srid << " -I " << shapefilePath << " " << tableName
                    << " | psql -U " << username << " -d " << dbName << " -h " << hostAddr
                    << " -p 5434";
            int status = system(command.str().c_str());

        // restoring stdout and stderr
#if defined(__linux__)
            stderr = freopen("/dev/tty", "a", stderr);
            stdout = freopen("/dev/tty", "a", stdout);
#elif defined(__WIN64) || defined(__WIN32)
            stdout = freopen("CON", "w", stdout);
            stderr = freopen("CON", "w" ,stderr);
#endif

            if (status != 0)
            {
                std::unique_ptr<std::fstream, std::function<void(std::fstream*)>> errorStream(new std::fstream("./.fd_stderr", std::ios::in), [](std::fstream *fd) -> void {
                    fd->close();
                });

                if (!errorStream)
                    throw std::runtime_error("couldn't open the error file");

                std::string errorLines = "";
                std::string tempError;

                while (std::getline(*errorStream, tempError, '\n'))
                    errorLines += tempError;

                throw std::runtime_error(errorLines);
            }

//  deleting the created the fd_stdout and fd_stderr files
#if defined(__linux__)
            system("rm -f ./.fd_stdout ./.fd_stderr");
#elif defined(__WIN32) || defined(__WIN64)
            system("del /f .fd_stdout .fd_stderr");
#endif

            QMessageBox::information(nullptr, "Successful!", "the operation was run successfully!");
        }
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
