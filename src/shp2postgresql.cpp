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
#include <postgresql/libpq-fe.h>.

ShapefileConversion::Shp2Postgresql::Shp2Postgresql(QWidget *parent)
    : QMainWindow(parent),
      _ui(std::make_unique<Ui::Shp2Postgresql>())
{
    this->_ui->setupUi(this);
    this->setFixedSize(this->geometry().width(), this->geometry().height());

    this->_ui->sridSpinBox->setMaximum(std::numeric_limits<int>::max());
    this->_ui->portSpinBox->setMaximum(65535);

    QObject::connect(this->_ui->shapefilePathBrowseButton, SIGNAL(clicked()), this, SLOT(slot_shpPathBrowseButton()));
    QObject::connect(this->_ui->cancelButton, SIGNAL(clicked()), this, SLOT(slot_cancelButtonClicked()));
    QObject::connect(this->_ui->importButton, SIGNAL(clicked()), this, SLOT(slot_importButtonClicked()));
}

bool ShapefileConversion::Shp2Postgresql::allSectionsFilled() const
{
    if (this->_ui->shapefilePathLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill shapefile path section");

    else if (this->_ui->dbNameLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill database name section");

    else if (this->_ui->tableNameLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill table name section");

    else if (this->_ui->hostIpAddrLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill host IP address section");

    else if (this->_ui->portSpinBox->text().isEmpty())
        throw std::runtime_error("You shoud fill target port number");

    else if (this->_ui->usernameLineEdit->text().isEmpty())
        throw std::runtime_error("You should fill Username Section");

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

void ShapefileConversion::Shp2Postgresql::changeConsoleStdStream()
{
    freopen("./.fd_stderr", "w", stderr);
    freopen("./.fd_stdout", "w", stdout);
}

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

void ShapefileConversion::Shp2Postgresql::removeConsoleStreamFiles()
{
#if defined(__linux__)
            system("rm -f ./.fd_stdout ./.fd_stderr");
#elif defined(__WIN32) || defined(__WIN64)
            system("del /f .fd_stdout .fd_stderr");
#endif
}

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
        if (this->allSectionsFilled() and this->userInputsAreValid())
        {
            std::string shapefilePath = this->_ui->shapefilePathLineEdit->text().toStdString();
            std::string srid = this->_ui->sridSpinBox->text().toStdString();
            std::string dbName = this->_ui->dbNameLineEdit->text().toStdString();
            std::string tableName = this->_ui->tableNameLineEdit->text().toStdString();
            std::string hostAddr = this->_ui->hostIpAddrLineEdit->text().toStdString();
            std::string portNumber = this->_ui->portSpinBox->text().toStdString();
            std::string username = this->_ui->usernameLineEdit->text().toStdString();

            if (this->isThereTable(dbName, tableName, hostAddr, portNumber, username))
            {
                QMessageBox::warning(nullptr, "Table Exists!", "The named table already exists in dbName->public(schema)");
                return;
            }

            this->changeConsoleStdStream();

            std::stringstream command;
            command << "shp2pgsql -s " << srid     << " -I " << shapefilePath << " "    << tableName
                    << " | psql -U "   << username << " -d " << dbName        << " -h " << hostAddr
                    << " -p "          << portNumber;
            int status = system(command.str().c_str());

            // restoring stdout and stderr
            this->restoreConsoleStdStream();

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
