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
#include <postgresql/libpq-fe.h>

ShapefileConversion::Shp2Postgresql::Shp2Postgresql(QWidget *parent)
    : QMainWindow(parent),
      _ui(std::make_unique<Ui::Shp2Postgresql>())
{
    _ui->setupUi(this);
    setFixedSize(geometry().width(), geometry().height());

    _ui->sridSpinBox->setMaximum(std::numeric_limits<int>::max());
    _ui->portSpinBox->setMaximum(65535);

    QObject::connect(_ui->shapefilePathBrowseButton, SIGNAL(clicked()), this, SLOT(slot_shpPathBrowseButton()));
    QObject::connect(_ui->cancelButton, SIGNAL(clicked()), this, SLOT(slot_cancelButtonClicked()));
    QObject::connect(_ui->importButton, SIGNAL(clicked()), this, SLOT(slot_importButtonClicked()));
}

bool ShapefileConversion::Shp2Postgresql::allSectionsFilled() const
{
    if (_ui->shapefilePathLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill shapefile path section");

    else if (_ui->dbNameLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill database name section");

    else if (_ui->tableNameLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill table name section");

    else if (_ui->hostIpAddrLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill host IP address section");

    else if (_ui->portSpinBox->text().isEmpty())
        throw std::runtime_error("You shoud fill target port number");

    else if (_ui->usernameLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill Username Section");

    return true;
}

bool ShapefileConversion::Shp2Postgresql::userInputsAreValid() const
{
    // Validating shapefile path (the input file exists or not)
    QFile file(_ui->shapefilePathLineEdit->text().trimmed());
    if (!file.exists())
        throw std::runtime_error("The input shapefile path doesn't exists");

    // validating input database name
    std::regex pgsqlValidDBNamePattern("^[A-Za-z][A-Za-z0-9_]{0,62}$");
    if (!std::regex_match(_ui->dbNameLineEdit->text().trimmed().toStdString(), pgsqlValidDBNamePattern))
        throw std::runtime_error("The input database name is not a valid syntax for database name in PostgreSQL");

    // validating input table name
    std::regex pgsqlValidTableNamePattern("^[A-Za-z][A-Za-z0-9_]{0,62}$");
    if (!std::regex_match(_ui->tableNameLineEdit->text().trimmed().toStdString(), pgsqlValidTableNamePattern))
        throw std::runtime_error("The input table name is not a valid syntax for table name in PostgreSQL");

    // validating ip address syntax
    // Regex to validate IP address format
    std::regex validIpv4Pattern("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    if (!std::regex_match(_ui->hostIpAddrLineEdit->text().trimmed().toStdString(), validIpv4Pattern))
        throw std::runtime_error("Invalid ipv4!");

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

    if (!std::regex_match(_ui->usernameLineEdit->text().trimmed().toStdString(), validUsernamePattern))
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
        _ui->shapefilePathLineEdit->setText(filePath);
}

void ShapefileConversion::Shp2Postgresql::slot_cancelButtonClicked()
{
    this->close();
}

// static member-function
void ShapefileConversion::Shp2Postgresql::changeConsoleStdStream()
{
    freopen("./.fd_stderr", "w", stderr);
    freopen("./.fd_stdout", "w", stdout);
}

// static member-function
void ShapefileConversion::Shp2Postgresql::restoreConsoleStdStream()
{
#if defined(__linux__)
    freopen("/dev/tty", "a", stderr);
    freopen("/dev/tty", "a", stdout);
#elif defined(__WIN64) || defined(__WIN32)
    freopen("CON", "w", stdout);
    freopen("CON", "w" ,stderr);
#endif
}

// static member-function
void ShapefileConversion::Shp2Postgresql::removeConsoleStreamFiles()
{
#if defined(__linux__)
            system("rm -f ./.fd_stdout ./.fd_stderr");
#elif defined(__WIN32) || defined(__WIN64)
            system("del /f .fd_stdout .fd_stderr");
#endif
}

// static member-function
bool ShapefileConversion::Shp2Postgresql::isThereTable(std::string const& dbName,
                                                       std::string const& tableName,
                                                       std::string const& hostAddr,
                                                       std::string const& port,
                                                       std::string const& username)
{
    bool result;

    PGconn          *conn;
    PGresult        *res;
    int             rec_count;
    int             row;
    int             col;

    std::stringstream connectionString;
    connectionString << "dbname=" << dbName << " host="  << hostAddr
                     << " port="  << port    << " user=" << username;

    conn = PQconnectdb(connectionString.str().c_str());

    if (PQstatus(conn) == CONNECTION_BAD)
        throw std::runtime_error("We were unable to connect to the database");

    std::stringstream query;
    query << "SELECT EXISTS ("
          << "SELECT FROM pg_catalog.pg_class c "
          << "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
          << "WHERE n.nspname = 'public' AND  c.relname = '" << tableName << "' AND c.relkind = 'r');";
    res = PQexec(conn, query.str().c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
        throw std::runtime_error("invalid query sent to postgresql (internal problem)");

    char* answer = PQgetvalue(res, 0, 0);
    if (strcmp(answer, "f") == 0)
        result = false;
    else if (strcmp(answer, "t") == 0)
        result = true;

    PQclear(res);
    PQfinish(conn);

    return result;
}

void ShapefileConversion::Shp2Postgresql::slot_importButtonClicked()
{
    try
    {
        if (allSectionsFilled() and userInputsAreValid())
        {
            std::string shapefilePath = _ui->shapefilePathLineEdit->text().toStdString();
            std::string srid = _ui->sridSpinBox->text().toStdString();
            std::string dbName = _ui->dbNameLineEdit->text().toStdString();
            std::string tableName = _ui->tableNameLineEdit->text().toStdString();
            std::string hostAddr = _ui->hostIpAddrLineEdit->text().toStdString();
            std::string portNumber = _ui->portSpinBox->text().toStdString();
            std::string username = _ui->usernameLineEdit->text().toStdString();

            if (isThereTable(dbName, tableName, hostAddr, portNumber, username))
            {
                QMessageBox::warning(nullptr, "Table Exists!", "The named table already exists in dbName->public(schema)");
                return;
            }

            changeConsoleStdStream();

            std::stringstream command;
            command << "shp2pgsql -s " << srid     << " -I " << shapefilePath << " "    << tableName
                    << " | psql -U "   << username << " -d " << dbName        << " -h " << hostAddr
                    << " -p "          << portNumber;
            int status = system(command.str().c_str());

            // restoring stdout and stderr
            restoreConsoleStdStream();

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

            QMessageBox::information(nullptr, "Successful!", "the operation was run successfully!");

            removeConsoleStreamFiles();
        }
    }
    catch(std::runtime_error const& ex)
    {
        if (ex.what() != NULL)
            QMessageBox::critical(nullptr, "Runtime Error", ex.what());
    }
    catch(std::exception const& ex)
    {
        if (ex.what() != NULL)
            QMessageBox::critical(nullptr, "Exception Occured!", ex.what());
    }
    catch(...)
    {
        QMessageBox::critical(nullptr, "Unknown Error!", "Something Went wrong");
    }
}
