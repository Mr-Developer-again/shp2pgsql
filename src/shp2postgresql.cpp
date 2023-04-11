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

    this->connections();
}

void ShapefileConversion::Shp2Postgresql::connections()
{
    QObject::connect(_ui->shapefilePathBrowseButton, SIGNAL(clicked()), this, SLOT(slot_shpPathBrowseButton()));
    QObject::connect(_ui->cancelPushButton, SIGNAL(clicked()), this, SLOT(slot_cancelButtonClicked()));
    QObject::connect(_ui->importPushButton, SIGNAL(clicked()), this, SLOT(slot_importButtonClicked()));
}

bool ShapefileConversion::Shp2Postgresql::allSectionsFilled() const
{
    if (_ui->shapefilePathLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill shapefile path section");

    return true;
}

bool ShapefileConversion::Shp2Postgresql::userInputsAreValid() const
{
    // Validating shapefile path (the input file exists or not)
    QFile file(_ui->shapefilePathLineEdit->text().trimmed());
    if (!file.exists())
        throw std::runtime_error("The input shapefile path doesn't exists");

    // validating input table name
    std::regex pgsqlValidTableNamePattern("^[A-Za-z][A-Za-z0-9_]{0,62}$");
    if (!std::regex_match(_ui->tableNameLineEdit->text().trimmed().toStdString(), pgsqlValidTableNamePattern))
        throw std::runtime_error("The input table name is not a valid syntax for table name in PostgreSQL");

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
            std::string tableName = _ui->tableNameLineEdit->text().toStdString();

            std::string srid = "4326";
            std::string username = "postgres";
            std::string dbName = "test";
            std::string hostAddr = "127.0.0.1";
            std::string portNumber = "5432";

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
