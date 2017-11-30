#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //ui->recoPushButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    commandCo = new TelnetConnection("localhost", 1023, "root", "clover");
    inputCo = new TelnetConnection("localhost", 1023, "root", "clover");
    canoeRunCo = new TelnetConnection("localhost", 1023, "root", "clover");
    inputDecoder = new InputDecoder();
    commandCo->debugName = "Command";
    inputCo->debugName = "Input";
    canoeRunCo->debugName = "Canoe";
    canoeRunning = false;
    existingSaveState.fill(false, 4);
    setButtonStatus();


    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimerTick()));
    connect(inputCo, SIGNAL(commandReturnedNewLine(QByteArray)), this, SLOT(onInputNewLine(QByteArray)));
    connect(commandCo, SIGNAL(connectionError(TelnetConnection::ConnectionError)), this, SLOT(onCommandCoConnectionError(TelnetConnection::ConnectionError)));
    connect(commandCo, SIGNAL(disconnected()), this, SLOT(onCommandCoDisconnected()));
    connect(inputDecoder, SIGNAL(buttonPressed(SNESButton)), this, SLOT(buttonPressed(SNESButton)));
    connect(this, SIGNAL(canoeStarted()), this, SLOT(onCanoeStarted()));
    connect(this, SIGNAL(canoeStopped()), this, SLOT(onCanoeStopped()));
    connect(commandCo, SIGNAL(connected()), this, SLOT(onCommandCoConnected()));
    connect(inputCo, SIGNAL(connected()), this, SLOT(onInputCoConnected()));
    spy = new QSignalSpy(commandCo, SIGNAL(connectionError(TelnetConnection::ConnectionError)));

    commandCo->conneect();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onTimerTick()
{
    if (commandCo->state() == TelnetConnection::Ready || commandCo->state() == TelnetConnection::Connected)
    {
        QByteArray result = commandCo->syncExecuteCommand("pidof canoe-shvc > /dev/null && echo 1 || echo 0");
        bool oldcr = canoeRunning;
        //qDebug() << result << result.trimmed();
        canoeRunning = (result.trimmed() == "1");
        qDebug() << canoeRunning;
        if (oldcr == false && canoeRunning == true)
            emit canoeStarted();
        if (oldcr == true && canoeRunning == false)
            emit canoeStopped();
        timer.setInterval(5000);
    }
}

void MainWindow::onCommandCoConnected()
{
    inputCo->conneect();
    inputCo->setOneCommandMode(true);
    canoeRunCo->conneect();
    timer.start(500);
}

void MainWindow::onInputNewLine(QByteArray input)
{
    //qDebug() << input;
    inputDecoder->decodeHexdump(input);
}


/*

  without savestate
canoe-shvc -rollback-snapshot-period 720 -rom /tmp/rom/skar_lttphack_jp_noss.sfrom --volume 100 -rollback-snapshot-period 600 --sram-file /var/lib/clover/profiles/0/CLV-U-XHDAN/cartridge.sram -filter 1 -magfilter 3 --save-time-path /var/cache/clover//volatile/CLV-U-XHDAN/suspendpoint0/state.time --wait-transition-fd 13 --start-transition-fd 14 --finish-transition-fd 15 --save-screenshot-on-quit /var/cache/clover//volatile/CLV-U-XHDAN/suspendpoint0/state.png -rollback-mode 1 --rollback-ui /usr/share/canoe/rollback-ui --enable-sram-file-hash -rollback-output-dir /var/cache/clover//volatile/CLV-U-XHDAN/suspendpoint0/rollback/


  with savestate
canoe-shvc -rollback-snapshot-period 720 -rom /tmp/rom/skar_lttphack_jp_noss.sfrom --volume 100 -rollback-snapshot-period 600 --sram-file /var/lib/clover/profiles/0/CLV-U-XHDAN/cartridge.sram -filter 1 -magfilter 3 --load-time-path /var/lib/clover/profiles/0/CLV-U-XHDAN/suspendpoint1//state.time --save-time-path /var/cache/clover//volatile/CLV-U-XHDAN/suspendpoint0/state.time --wait-transition-fd 13 --start-transition-fd 14 --finish-transition-fd 15 --save-screenshot-on-quit /var/cache/clover//volatile/CLV-U-XHDAN/suspendpoint0/state.png -rollback-mode 1 --rollback-ui /usr/share/canoe/rollback-ui --enable-sram-file-hash -rollback-output-dir /var/cache/clover//volatile/CLV-U-XHDAN/suspendpoint0/rollback/ -rollback-input-dir /var/lib/clover/profiles/0/CLV-U-XHDAN/suspendpoint1/rollback/


--save-time-path /var/cache/clover//volatile/CLV-U-XHDAN/suspendpoint0/state.time

--load-time-path /var/lib/clover/profiles/0/CLV-U-XHDAN/suspendpoint1//state.time

*/

void MainWindow::onCanoeStarted()
{
    ui->snesStatusLabel->setText("SNES emulator running");
    QByteArray ba = commandCo->syncExecuteCommand("ps | grep canoe | grep -v grep");
    QString result = ba.trimmed();
    QString canoeStr = result.mid(result.indexOf("canoe"));
    QStringList canoeArgs = canoeStr.split(" ");
    int sramArg = canoeArgs.indexOf("--sram-file");
    if (sramArg == -1)
        return;
    currentCanoeExecution = canoeArgs;
    QString sramPath = canoeArgs[sramArg + 1];
    ba = commandCo->syncExecuteCommand("ls " + QFileInfo(sramPath).absolutePath().mid(2) + " | grep susp");
    result = ba.trimmed();
    QStringList saves = result.split("\r\n");
    if (!saves[0].isEmpty())
    {
        foreach(QString ss, saves)
        {
            existingSaveState[ss.right(1).toUInt() - 1] =  true;
        }
    }
    qDebug() << existingSaveState;
    setButtonStatus();
}

void MainWindow::onCanoeStopped()
{
    existingSaveState.fill(false, 4);
    setButtonStatus();
    ui->snesStatusLabel->setText("SNES emulator stopped");
}

void MainWindow::buttonPressed(SNESButton button)
{
    if (!canoeRunning)
        return;
    switch (button) {
    case SNESButton::A:
        executeCanoe(4);
        break;
    case SNESButton::B:
        executeCanoe(1);
        break;
    case SNESButton::Y:
        executeCanoe(2);
        break;
    case SNESButton::X:
        executeCanoe(3);
        break;
    default:
        break;
    }

}

void MainWindow::onCommandCoConnectionError(TelnetConnection::ConnectionError err)
{
    qDebug() << "Co error";
    if (err == TelnetConnection::SNESNotRunning)
        ui->snesStatusLabel->setText("The snes classic is not running or not connected to the computer");
    if (err == TelnetConnection::ConnectionRefused)
        ui->snesStatusLabel->setText("Hakchi2 is not running or it's not running its telenet server");
    timer.stop();
    ui->recoPushButton->setEnabled(true);
}

void MainWindow::onCommandCoDisconnected()
{
    timer.stop();
    ui->recoPushButton->setEnabled(true);
}

void MainWindow::onInputCoConnected()
{
    inputCo->executeCommand("hexdump -v -e '32/1 \"%02X\" \"\\n\"' /dev/input/by-path/platform-twi.2-event-joystick");
}

void MainWindow::executeCanoe(unsigned int saveNumber)
{
    if (!existingSaveState[saveNumber - 1])
        return;
    qDebug() << "Loading save : " << saveNumber;
    int i = currentCanoeExecution.indexOf("--sram-file");
    QString sramPath = currentCanoeExecution[i + 1];
    QString rollback = sramPath;
    sramPath.replace("cartridge.sram", QString("suspendpoint%1/state.time").arg(saveNumber));
    rollback.replace("cartridge.sram", QString("suspendpoint%1/rollback/").arg(saveNumber));
    int pos;
    if ((pos = currentCanoeExecution.indexOf("--load-time-path")) != -1)
    {
        currentCanoeExecution.removeAt(pos);
        currentCanoeExecution.removeAt(pos);
    }
    if ((pos = currentCanoeExecution.indexOf("-rollback-input-dir")) != -1)
    {
        currentCanoeExecution.removeAt(pos);
        currentCanoeExecution.removeAt(pos);
    }
    currentCanoeExecution.append("-rollback-input-dir");
    currentCanoeExecution.append(rollback);
    currentCanoeExecution.append("--load-time-path");
    currentCanoeExecution.append(sramPath);
    commandCo->syncExecuteCommand("kill -2 `pidof canoe-shvc`");
    qDebug() << "Canoe KILLED";
    commandCo->syncExecuteCommand("kill -2 `pidof  ReedPlayer-Clover` && sleep 2");
    canoeRunCo->executeCommand(currentCanoeExecution.join(" "));
    //qDebug() << "Started : " << currentCanoeExecution.join(" ");

}

void MainWindow::closeEvent(QCloseEvent *)
{
    commandCo->syncExecuteCommand("killall hexdump");
    inputCo->close();
    commandCo->close();
    canoeRunCo->close();
    qDebug() << spy->count();
}

void MainWindow::keyPressEvent(QKeyEvent *ev)
{
    if (!canoeRunning)
        return;
    if (ev->key() == Qt::Key_F1)
        executeCanoe(1);
    if (ev->key() == Qt::Key_F2)
        executeCanoe(2);
    if (ev->key() == Qt::Key_F3)
        executeCanoe(3);
    if (ev->key() == Qt::Key_F4)
        executeCanoe(4);
}

void MainWindow::setButtonStatus()
{
    unsigned int i = 0;
    foreach(bool b, existingSaveState)
    {
        if (i == 0)
            ui->BLabel->setEnabled(b);
        if (i == 1)
            ui->YLabel->setEnabled(b);
        if (i == 2)
            ui->XLabel->setEnabled(b);
        if (i == 3)
            ui->ALabel->setEnabled(b);
        i++;
    }
}



void MainWindow::on_recoPushButton_clicked()
{
    inputCo->conneect();
    ui->recoPushButton->setEnabled(false);
}
