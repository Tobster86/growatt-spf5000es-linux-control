#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <qdatetime.h>

extern "C" {
#include "tcpclient.h"
#include "spf5000es_defs.h"
#include "comms_defs.h"
}

MainWindow::MainWindow(QWidget *parent, SystemStatus* statusPtr)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    status = statusPtr;
    nSystemState = SYSTEM_STATE_NO_CHANGE;
    uiState = UIState::Information;

    connect(ui->btnBoostCharge, &QPushButton::clicked, this, &MainWindow::BoostCharge_clicked);
    connect(ui->btnSwitchToBatts, &QPushButton::clicked, this, &MainWindow::SwitchToBatts_clicked);
    connect(ui->btnSwitchToGrid, &QPushButton::clicked, this, &MainWindow::SwitchToGrid_clicked);
    connect(ui->btnGeneral, &QPushButton::clicked, this, &MainWindow::General_clicked);
    connect(ui->btnCharging, &QPushButton::clicked, this, &MainWindow::Charging_clicked);
    connect(ui->btnHealth, &QPushButton::clicked, this, &MainWindow::Health_clicked);
    connect(ui->btnAC, &QPushButton::clicked, this, &MainWindow::AC_clicked);
    connect(ui->btnBatteries, &QPushButton::clicked, this, &MainWindow::Batteries_clicked);

    PrepareStateChange();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::PrepareStateChange()
{
    ui->btnSwitchToGrid->setVisible(false);
    ui->btnSwitchToBatts->setVisible(false);
    ui->btnBoostCharge->setVisible(false);

    ui->lblSystemStatus->setVisible(false);

    ui->lblInverterStatus->setText("Changing Mode...");

    ui->lbl01->setVisible(false);
    ui->lbl02->setVisible(false);
    ui->lbl03->setVisible(false);
    ui->lbl04->setVisible(false);
    ui->lbl05->setVisible(false);
    ui->lbl06->setVisible(false);
    ui->lbl07->setVisible(false);
    ui->lbl08->setVisible(false);
}

void MainWindow::updateStatus()
{
    if(nSystemState != status->nSystemState)
    {
        PrepareStateChange();

        switch (status->nSystemState)
        {
            case SYSTEM_STATE_PEAK:
            {
                ui->btnSwitchToGrid->setVisible(true);
                ui->btnBoostCharge->setVisible(true);
                ui->lblSystemStatus->setText("Peak (Batteries)");
            }
            break;

            case SYSTEM_STATE_BYPASS:
            {
                ui->btnSwitchToBatts->setVisible(true);
                ui->btnBoostCharge->setVisible(true);
                ui->lblSystemStatus->setText("Bypass (Grid)");
            }
            break;

            case SYSTEM_STATE_OFF_PEAK:
            {
                ui->lblSystemStatus->setText("Off-Peak (Grid, Charging)");
            }
            break;

            case SYSTEM_STATE_BOOST:
            {
                ui->btnSwitchToGrid->setVisible(true);
                ui->btnSwitchToBatts->setVisible(true);
                ui->lblSystemStatus->setText("Boost Charging (Grid)");
            }
            break;
        }

        ui->lblSystemStatus->setVisible(true);
    }

    switch (status->nInverterState)
    {
        case STANDBY:
            ui->lblInverterStatus->setText("Inverter Status: Standby");
            break;
        case NO_USE:
            ui->lblInverterStatus->setText("Inverter Status: No Use");
            break;
        case DISCHARGE:
            ui->lblInverterStatus->setText("Inverter Status: Discharge");
            break;
        case FAULT:
            ui->lblInverterStatus->setText("Inverter Status: Fault");
            break;
        case FLASH:
            ui->lblInverterStatus->setText("Inverter Status: Flash");
            break;
        case PV_CHG:
            ui->lblInverterStatus->setText("Inverter Status: PV Charging");
            break;
        case AC_CHG:
            ui->lblInverterStatus->setText("Inverter Status: AC Charging");
            break;
        case COM_CHG:
            ui->lblInverterStatus->setText("Inverter Status: Combined Charging");
            break;
        case COM_CHG_BYP:
            ui->lblInverterStatus->setText("Inverter Status: Combined Charging with Bypass");
            break;
        case PV_CHG_BYP:
            ui->lblInverterStatus->setText("Inverter Status: PV Charging with Bypass");
            break;
        case AC_CHG_BYP:
            ui->lblInverterStatus->setText("Inverter Status: AC Charging with Bypass");
            break;
        case BYPASS:
            ui->lblInverterStatus->setText("Inverter Status: Bypass");
            break;
        case PV_CHG_DISCHG:
            ui->lblInverterStatus->setText("Inverter Status: PV Charging and Discharging");
            break;
        default:
            ui->lblInverterStatus->setText("Inverter Status: Invalid State");
            break;
    }

    switch(uiState)
    {
        case UIState::Information:
        {
            ui->lbl01->setVisible(true);
            ui->lbl02->setVisible(true);
            ui->lbl03->setVisible(true);
            ui->lbl04->setVisible(true);
            ui->lbl05->setVisible(true);
            ui->lbl06->setVisible(true);

            ui->lbl01->setText(QString("Load Percent: %1%").arg(status->nLoadPercent / 10.0));
            ui->lbl02->setText(QString("Output Watts: %1 W").arg(status->nOutputWatts / 10.0));
            ui->lbl03->setText(QString("Output Apparent Power: %1 VA").arg(status->nOutputApppwr / 10.0));
            ui->lbl04->setText(QString("AC Use Watts: %1 W").arg(status->nAcUseWatts / 10.0));
            ui->lbl05->setText(QString("Battery Use Today: %1 kWh").arg(status->nBattuseToday / 10.0));
            ui->lbl06->setText(QString("AC Use Today: %1 kWh").arg(status->nAcUseToday / 10.0));
        }
        break;

        case UIState::Charging:
        {
            QString strChargeComplete = QDateTime::fromSecsSinceEpoch(status->slOffPeakChgComplete).toString("HH:mm:ss");

            ui->lbl01->setVisible(true);
            ui->lbl02->setVisible(true);
            ui->lbl03->setVisible(true);
            ui->lbl04->setVisible(true);
            ui->lbl05->setVisible(true);

            ui->lbl01->setText(QString("Planned Charging Amps: %1 A").arg(utils_GetOffpeakChargingAmps(&status)));
            ui->lbl02->setText(QString("Off-peak Charge Completed: %1").arg(strChargeComplete));
            ui->lbl03->setText(QString("Off-peak Charge Amount: %1 kWh").arg(status->nOffPeakChargeKwh / 10.0));
            ui->lbl04->setText(QString("AC Charge Energy Today: %1 kWh").arg(status->nAcchgegyToday / 10.0));
            ui->lbl05->setText(QString("Battery Charge Amps: %1 A").arg(status->nBattchgAmps / 10.0));
        }
        break;

        case UIState::Health:
        {
            ui->lbl01->setVisible(true);
            ui->lbl02->setVisible(true);
            ui->lbl03->setVisible(true);
            ui->lbl04->setVisible(true);
            ui->lbl05->setVisible(true);
            ui->lbl06->setVisible(true);
            ui->lbl07->setVisible(true);
            ui->lbl08->setVisible(true);

            ui->lbl01->setText(QString("Inverter Temperature: %1 °C").arg(status->nInverterTemp / 10.0));
            ui->lbl02->setText(QString("DCDC Temperature: %1 °C").arg(status->nDCDCTemp / 10.0));
            ui->lbl03->setText(QString("Buck 1 Temperature: %1 °C").arg(status->nBuck1Temp / 10.0));
            ui->lbl04->setText(QString("Buck 2 Temperature: %1 °C").arg(status->nBuck2Temp / 10.0));
            ui->lbl05->setText(QString("Inverter Fan Speed: %1%").arg(status->nInvFanspeed));
            ui->lbl06->setText(QString("Bus Volts: %1 V").arg(status->nBusVolts / 100.0));
            ui->lbl07->setText(QString("Output Amps: %1 A").arg(status->nOutputAmps / 10.0));
            ui->lbl08->setText(QString("Inverter Amps: %1 A").arg(status->nInverterAmps / 10.0));
        }
        break;

        case UIState::AC:
        {
            ui->lbl01->setVisible(true);
            ui->lbl02->setVisible(true);
            ui->lbl03->setVisible(true);
            ui->lbl04->setVisible(true);
            ui->lbl05->setVisible(true);

            ui->lbl01->setText(QString("Grid Volts: %1 V").arg(status->nGridVolts / 10.0));
            ui->lbl02->setText(QString("Grid Frequency: %1 Hz").arg(status->nGridFreq / 100.0));
            ui->lbl03->setText(QString("AC Output Volts: %1 V").arg(status->nAcOutVolts / 10.0));
            ui->lbl04->setText(QString("AC Output Frequency: %1 Hz").arg(status->nAcOutFreq / 100.0));
            ui->lbl05->setText(QString("AC Input Watts: %1 W").arg(status->nAcInputWattsL / 10.0));
        }
        break;

        case UIState::Batteries:
        {
            ui->lbl01->setVisible(true);
            ui->lbl02->setVisible(true);
            ui->lbl03->setVisible(true);
            ui->lbl04->setVisible(true);

            ui->lbl01->setText(QString("Battery Volts: %1 V").arg(status->nBatteryVolts / 100.0));
            ui->lbl02->setText(QString("AC Charge Watts: %1 W").arg(status->nAcChargeWattsL / 10.0));
            ui->lbl03->setText(QString("Battery Use Watts: %1 W").arg(status->nBattUseWatts / 10.0));
            ui->lbl04->setText(QString("Battery Watts: %1 W").arg(status->nBattWatts / 10.0));
        }
        break;
    }
}

void MainWindow::BoostCharge_clicked()
{
    PrepareStateChange();
    tcpclient_SendCommand(COMMAND_REQUEST_BOOST);
}

void MainWindow::SwitchToBatts_clicked()
{
    PrepareStateChange();
    tcpclient_SendCommand(COMMAND_REQUEST_BATTS);
}

void MainWindow::SwitchToGrid_clicked()
{
    PrepareStateChange();
    tcpclient_SendCommand(COMMAND_REQUEST_GRID);
}

void MainWindow::General_clicked()
{
    uiState = UIState::Information;
    PrepareStateChange();
}

void MainWindow::Charging_clicked()
{
    uiState = UIState::Charging;
    PrepareStateChange();
}

void MainWindow::Health_clicked()
{
    uiState = UIState::Health;
    PrepareStateChange();
}

void MainWindow::AC_clicked()
{
    uiState = UIState::AC;
    PrepareStateChange();
}

void MainWindow::Batteries_clicked()
{
    uiState = UIState::Batteries;
    PrepareStateChange();
}
