#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "inputdecoder.h"
#include "telnetconnection.h"

#include <QKeyEvent>
#include <QMainWindow>
#include <QSignalSpy>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


signals:
    void    canoeStarted();
    void    canoeStopped();

private slots:
    void    onTimerTick();
    void    onCommandCoConnected();
    void    onInputNewLine(QByteArray);
    void    onCanoeStarted();
    void    onCanoeStopped();
    void    buttonPressed(SNESButton button);
    void    onCommandCoConnectionError(TelnetConnection::ConnectionError err);
    void    onCommandCoDisconnected();
    void    onInputCoConnected();


    void on_recoPushButton_clicked();

private:
    Ui::MainWindow *ui;

    QTimer              timer;
    TelnetConnection*   commandCo;
    TelnetConnection*   inputCo;
    TelnetConnection*   canoeRunCo;
    InputDecoder*       inputDecoder;
    bool                canoeRunning;
    QVector<bool>       existingSaveState;
    QStringList         currentCanoeExecution;
    QSignalSpy*         spy;

    void                setButtonStatus();
    void                executeCanoe(unsigned int saveNumber);

    void            closeEvent(QCloseEvent *);
    void            keyPressEvent(QKeyEvent* ev);
};

#endif // MAINWINDOW_H
