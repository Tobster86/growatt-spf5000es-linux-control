#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

extern "C" {
#include "system_defs.h"
}

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr, SystemStatus* statusPtr = nullptr);
    ~MainWindow();

public slots:
    void updateStatus();

private slots:
    void BoostCharge_clicked();
    void SwitchToBatts_clicked();
    void SwitchToGrid_clicked();

private:
    void PrepareStateChange();
    Ui::MainWindow *ui;
    SystemStatus *status;
    uint16_t nSystemState;
};
#endif // MAINWINDOW_H
