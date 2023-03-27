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
