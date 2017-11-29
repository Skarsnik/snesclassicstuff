#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <telnetconnection.h>
#include "rommapping.h"


struct meminfo {
    unsigned int start;
    unsigned int size;
    QString flags;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
    QFile file("D:\\Emulation\\Super Metroid\\Super Metroid.smc");
    if (!file.open(QIODevice::ReadOnly))
    {
        return 1;
    }
    QString memstuff = "/var/lib/hakchi/rootfs/memstuff";


    //  hexdump -v -e '32/1 "%02X" "\n"' /dev/input/by-path/platform-twi.1-event-joystick

    TelnetConnection telco("localhost", 1023, "root", "clover");
    telco.conneect();
    QEventLoop loop;
    QObject::connect(&telco, SIGNAL(connected()), &loop, SLOT(quit()));
    loop.exec();
    QByteArray ba = telco.syncExecuteCommand("pidof canoe-shvc");
    qDebug() << "Received : " << ba.trimmed();
    QString pid = ba.trimmed();

    telco.syncExecuteCommand("hexdump -v -e '32/1 \"%02X\" \"\\n\"' /dev/input/by-path/platform-twi.1-event-joystick");
    ba = telco.syncExecuteCommand("cat /proc/" + pid + "/maps | grep heap");
    QString sStartHeap = ba.split(' ')[0].split('-')[0];
    bool ok;
    unsigned int startHeap = sStartHeap.toUInt(&ok, 16);
    QString sEndHeap = ba.split(' ')[0].split('-')[1];
    unsigned int endHeap = sEndHeap.toUInt(&ok, 16);
    unsigned int heapSize = endHeap - startHeap;
    qDebug() << "heapsize" << heapSize;

    ba = telco.syncExecuteCommand("cat /proc/" + pid + "/maps");
    QString bas = ba;
    QStringList canoeMaps = bas.split("\r\n");
    QList<struct meminfo> blankEntries;
    qDebug() << canoeMaps.size();
    foreach (QString memEntry, canoeMaps)
    {
        QStringList infos = memEntry.split(QRegExp("\\s+"));
        if (infos.size() > 5 && infos[5].isEmpty())
        {
            struct meminfo newInfo;
            newInfo.start = infos[0].split('-')[0].toUInt(&ok, 16);
            newInfo.size = infos[0].split('-')[1].toUInt(&ok, 16) - newInfo.start;
            newInfo.flags = infos[1];
            blankEntries.append(newInfo);
            qDebug() << "Info map : " << infos;
        }
    }
    foreach (struct meminfo memI, blankEntries) {
        if (memI.flags.contains('w'))
        {
            qDebug() << memI.start << memI.size;
        }
    }
    return 1;
    QString startAddr = QString::number(3052400640, 16);
    // Searching for
    for (unsigned int i = 0; i < 0x3F; i++)
    {
        int pc_addr = rommapping_snes_to_pc((i << 16) + 0x8000, LoROM, true);
        file.seek(pc_addr);
        QByteArray buf = file.read(100);
        QString toSearch;
        for (unsigned j = 0; j < buf.size(); j++)
            toSearch.append(QString::number((unsigned char) buf.at(i), 16));
        ba = telco.syncExecuteCommand(memstuff + " " + pid + " search " + startAddr + " " + "8392704 " + toSearch);
        qDebug() << "Bank " + QString::number(i, 16) << toSearch << ba;
    }

    return a.exec();
}
