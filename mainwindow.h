#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // Load a file with the given name
    void loadFile(const QString &fileName);

    // TODO: delme?
    bool hasFile();

    void updateWindowMenu();

private slots:
    void on_actionLoad_GCode_triggered();

    void on_LayerHeight_sliderMoved(int position);

    void openRecentFile();
    void on_actionClose_triggered();


    void on_zoomIn_clicked();
    void on_zoomOut_clicked();

    void on_panLeft_clicked();

    void on_panRight_clicked();

    void on_panUp_clicked();

    void on_panDown_clicked();

    void on_buttonConfigBrowse_clicked();

   // void on_button3dModelBrowse_clicked();


    void on_LayerMin_valueChanged(int value);

    void on_actionSave_gcode_triggered();



    void on_checkBoxSurfs_toggled(bool checked);

    void on_checkBoxRoofs_toggled(bool checked);

    void on_checkBoxFloors_toggled(bool checked);

    void on_checkBoxLoops_toggled(bool checked);

    void on_checkBoxInfills_toggled(bool checked);

    void on_actionOpen_3D_model_triggered();

    void on_pushButtonSlice_clicked();

private:
    Ui::MainWindow *ui;

    QString curFile;

    void setCurrentFile(const QString &fileName);
    void updateRecentFileActions();
    QString strippedName(const QString &fullFileName);

    enum { MaxRecentFiles = 10 };
    QAction *recentFileActs[MaxRecentFiles];

    void sliceModelAndCreateToolPaths(const char* filename);
};

#endif // MAINWINDOW_H
