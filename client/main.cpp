#include "mainwindow.h"

#include <QApplication>
#include <QThread>
#include <QDebug>
#include <QQuickWidget>
#include <QFile>

extern "C" {
#include "tcpclient.h"
#include "comms_defs.h"
#include "system_defs.h"
}

bool bQuit = false;
struct SystemStatus status;
MainWindow *mainWindow = nullptr;

void _tcpclient_ReceiveStatus(uint8_t* pStatus, uint32_t lLength)
{
    memcpy(&status, pStatus, lLength);

    //Call Qt to update UI with the new status.
    if (mainWindow)
    {
        QMetaObject::invokeMethod(mainWindow, "updateStatus", Qt::QueuedConnection);
    }
}

void networkThread() {
    // Initialize the TCP client and set the callback
    if (!tcpclient_init())
    {
        qDebug() << "Failed to initialize TCP client";
        return;
    }

    // Run the network operations in a loop
    while (!bQuit) {
        if(tcpclient_GetConnected())
        {
            tcpclient_SendCommand(COMMAND_REQUEST_STATUS);
        }

        // Sleep or perform other operations
        QThread::sleep(1);  // Adjust sleep as needed
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    bool bQtCreator = (std::getenv("GIO_LAUNCHED_DESKTOP_FILE") != nullptr);

    MainWindow w(nullptr, &status);
    mainWindow = &w;

    if(bQtCreator)
        w.show();
    else
        w.showFullScreen();

    // Create a thread
    QThread workerThread;

    // Move the worker function to the thread
    QObject::connect(&workerThread, &QThread::started, []()
    {
        networkThread();  // Call the worker function
    });

    // Start the worker thread
    workerThread.start();

    int result = a.exec();
    bQuit = true;

    // Cleanup
    workerThread.quit();
    workerThread.wait();

    return result;
}
