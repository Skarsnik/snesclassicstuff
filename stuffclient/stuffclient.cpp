
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_stuffClient, "StuffClient")

#define sDebug() qCDebug(log_stuffClient)

#include "stuffclient.h"
#define SNESCLASSIC_IP "169.254.13.37"

StuffClient::StuffClient(QObject *parent) : QObject(parent)
{
    socket = new QTcpSocket();
    streamMode = false;
    QObject::connect(socket, &QTcpSocket::connected, this, &StuffClient::connected);
}

bool StuffClient::connect()
{
    socket->connectToHost(SNESCLASSIC_IP, 1042);
    return true;
    //return socket->waitForConnected(100);
}

QByteArray StuffClient::waitForCommand(QByteArray cmd)
{
    QByteArray toret;
    sDebug() << "Executing command" << cmd;
    writeSocket("CMD " + cmd + "\n");
    if (!socket->waitForReadyRead(100))
        return QByteArray();
    forever {
            QByteArray data = socket->readAll();
            sDebug() << "Reading" << data;
            if (data.isEmpty())
                break;
            toret += data;
            if (!socket->waitForReadyRead(50))
                break;
        }
    toret.truncate(toret.size() - 4);
    return toret;
}

void StuffClient::detachedCommand(QByteArray cmd)
{
    sDebug() << "Running a detached command " << cmd;
    writeSocket("CMD_DETACHED " + cmd + "\n");
}

void StuffClient::streamFile(QByteArray filePath)
{
    writeSocket("STREAM_FILE " + filePath + "\n");
    QObject::connect(socket, &QTcpSocket::readyRead, this, &StuffClient::onReadyRead);
}

// Savestate Files are like 500Kbytes, should not take that long

void StuffClient::sendFile(QByteArray filePath, QByteArray data)
{
    writeSocket("PUT_FILE " + QByteArray::number(data.size()) + " " + filePath + "\n");
    writeSocket(data);
    socket->waitForBytesWritten();
}

// This return 10 bytes of size then the data;

QByteArray StuffClient::getFile(QByteArray filePath)
{
    QByteArray toret;
    writeSocket("GET_FILE " + filePath + "\n");
    socket->waitForReadyRead(50);
    QByteArray sizeData = socket->read(10);
    off_t size = 0;
    for (unsigned int i = 0; i < 10; i++)
    {
        size |= sizeData.at(i) << 8 * (9 - i);
    }
    sDebug() << "File size is " << size;
    socket->waitForReadyRead(50);
    forever {
            QByteArray data = socket->readAll();
            sDebug() << "Reading" << data;
            toret += data;
            if (toret.size() == size)
                break;
            if (!socket->waitForReadyRead(50))
                break;
        }
    return toret;
}

void StuffClient::close()
{
    socket->disconnect();
    socket->waitForDisconnected(100);
}

void StuffClient::onReadyRead()
{
    sDebug() << "Ready read";
    emit newFileData(socket->readAll());
}

bool StuffClient::isConnected()
{
    return socket->state() == QAbstractSocket::ConnectedState;
}

void    StuffClient::writeSocket(QByteArray data)
{
    sDebug() << "Writing " << data.size() << "bytes";
    socket->write(data);
}
