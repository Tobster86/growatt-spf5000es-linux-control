#include "mainwindow.h"
#include "./ui_mainwindow.h"

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

    connect(ui->btnBoostCharge, &QPushButton::clicked, this, &MainWindow::BoostCharge_clicked);
    connect(ui->btnSwitchToBatts, &QPushButton::clicked, this, &MainWindow::SwitchToBatts_clicked);
    connect(ui->btnSwitchToGrid, &QPushButton::clicked, this, &MainWindow::SwitchToGrid_clicked);

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

    ui->lblPeakMode->setVisible(false);
    ui->lblOffPeakMode->setVisible(false);
    ui->lblBypassMode->setVisible(false);
    ui->lblBoostMode->setVisible(false);

    ui->lblInverterStatus->setText("Changing Mode...");
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
                ui->lblPeakMode->setVisible(true);
            }
            break;

            case SYSTEM_STATE_BYPASS:
            {
                ui->btnSwitchToBatts->setVisible(true);
                ui->btnBoostCharge->setVisible(true);
                ui->lblBypassMode->setVisible(true);
            }
            break;

            case SYSTEM_STATE_OFF_PEAK:
            {
                ui->lblOffPeakMode->setVisible(true);
            }
            break;

            case SYSTEM_STATE_BOOST:
            {
                ui->btnSwitchToGrid->setVisible(true);
                ui->btnSwitchToBatts->setVisible(true);
                ui->lblBoostMode->setVisible(true);
            }
            break;
        }
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

    ui->lbl03->setText(QString("Inverter State: %1").arg(status->nInverterState));
    ui->lbl04->setText(QString("Output Watts: %1 W").arg(status->nOutputWatts / 10.0));
    ui->lbl05->setText(QString("Output Apparent Power: %1 VA").arg(status->nOutputApppwr / 10.0));
    ui->lbl07->setText(QString("AC Charge Watts (Low): %1 W").arg(status->nAcChargeWattsL / 10.0));
    ui->lbl08->setText(QString("Battery Volts: %1 V").arg(status->nBatteryVolts / 100.0));
    ui->lbl09->setText(QString("Bus Volts: %1 V").arg(status->nBusVolts / 100.0));
    ui->lbl10->setText(QString("Grid Volts: %1 V").arg(status->nGridVolts / 10.0));
    ui->lbl11->setText(QString("Grid Frequency: %1 Hz").arg(status->nGridFreq / 100.0));
    ui->lbl12->setText(QString("AC Output Volts: %1 V").arg(status->nAcOutVolts / 10.0));
    ui->lbl13->setText(QString("AC Output Frequency: %1 Hz").arg(status->nAcOutFreq / 100.0));
    ui->lbl15->setText(QString("Inverter Temperature: %1 °C").arg(status->nInverterTemp / 10.0));
    ui->lbl16->setText(QString("DCDC Temperature: %1 °C").arg(status->nDCDCTemp / 10.0));
    ui->lbl17->setText(QString("Load Percent: %1%").arg(status->nLoadPercent / 10.0));
    ui->lbl20->setText(QString("Buck 1 Temperature: %1 °C").arg(status->nBuck1Temp / 10.0));
    ui->lbl21->setText(QString("Buck 2 Temperature: %1 °C").arg(status->nBuck2Temp / 10.0));
    ui->lbl22->setText(QString("Output Amps: %1 A").arg(status->nOutputAmps / 10.0));
    ui->lbl23->setText(QString("Inverter Amps: %1 A").arg(status->nInverterAmps / 10.0));
    ui->lbl25->setText(QString("AC Input Watts (Low): %1 W").arg(status->nAcInputWattsL / 10.0));
    ui->lbl26->setText(QString("AC Charge Energy Today: %1 kWh").arg(status->nAcchgegyToday / 10.0));
    ui->lbl27->setText(QString("Battery Use Today: %1 kWh").arg(status->nBattuseToday / 10.0));
    ui->lbl28->setText(QString("AC Use Today: %1 kWh").arg(status->nAcUseToday / 10.0));
    ui->lbl29->setText(QString("AC Battery Charge Amps: %1 A").arg(status->nAcBattchgAmps / 10.0));
    ui->lbl30->setText(QString("AC Use Watts: %1 W").arg(status->nAcUseWatts / 10.0));
    ui->lbl31->setText(QString("Battery Use Watts: %1 W").arg(status->nBattuseWatts / 10.0));
    ui->lbl32->setText(QString("Battery Watts: %1 W").arg(status->nBattWatts / 10.0));
    ui->lbl34->setText(QString("Inverter Fan Speed: %1%").arg(status->nInvFanspeed));
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
