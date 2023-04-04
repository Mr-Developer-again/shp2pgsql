#ifndef SHAPEFILECONVERSION_SHP2POSTGRESQL_H
#define SHAPEFILECONVERSION_SHP2POSTGRESQL_H

#include <../res/ui_shp2postgresql.h>

#include <QMainWindow>

#include <memory>

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
        virtual ~Shp2Postgresql() = default;

    protected:
        /// @brief This method checks whether all sections in the form
        ///        are filled or not .
        /// @return If all fields have been filled, this method returns true,
        ///         otherwise, it throws exception (std::runtime_error
        ///         exception) and says which field hasn't been filled.
        bool allSectionsFilled() const;

        /// @brief This method checks whether user inputs are valid or not
        ///        (user input means the values which user has written in
        ///        the form sections) .
        /// @return Like "allSectionFilled()" method .
        bool userInputsAreValid() const;

        /// @brief This method changes console's standard stream (in the other word,
        ///        this method changes flow of stdout and stderr and guides them to files
        ///        instead of console)
        static void changeConsoleStdStream();

        /// @brief This method acts opposite of the method above (this method, guides stdout and
        ///        stderr to console instead of files)
        static void restoreConsoleStdStream();

        /// @brief This method deletes the files which "changeConosleStdStream()" method creates
        static void removeConsoleStreamFiles();

        /// @brief This method checks whether the user input table-name, does exist in the named dbName or not
        /// @note This method only checks "public" schema to find the table-name
        /// @return It returns "true" if the table does exist in the named dbName, otherwise, it returns "false"
        /// @param  these parameters are necessary for connecting and sending query to postgresql
        static bool isThereTable(std::string const& dbName,
                          std::string const& tableName,
                          std::string const& hostAddr,
                          std::string const& port,
                          std::string const& username);

    private:
        std::unique_ptr<Ui::Shp2Postgresql> _ui;

    private slots:
        /// @brief When "Browse" push button (shapefile path field) clicked,
        ///        this slot will be invoked
        void slot_shpPathBrowseButton();

        /// @brief When "Cancel" push button clicked, this slot will be invoked
        void slot_cancelButtonClicked();

        /// @brief When "Import" push button clicked, this slot will be invoked
        void slot_importButtonClicked();
    };

}
#endif // SHAPEFILECONVERSION_SHP2POSTGRESQL_H
