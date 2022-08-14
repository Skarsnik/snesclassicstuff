
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
    QObject::connect(socket, &QTcpSocket::stateChanged, this, [=](QAbstractSocket::SocketState state) {
        sDebug() << "State changed to " << state;
        ;
    });
    QObject::connect(socket, &QTcpSocket::errorOccurred, this, [=](QAbstractSocket::SocketError err) {
        sDebug() << "Error" << err;
        ;
    });
    QObject::connect(socket, &QTcpSocket::readyRead, this, &StuffClient::onReadyRead);
    m_classicIp = SNESCLASSIC_IP;
}

StuffClient::StuffClient(QString snesclassicIp, QObject* parent) : StuffClient(parent)
{
    m_classicIp = snesclassicIp;
}

bool StuffClient::connect()
{
    if (socket->state() != QAbstractSocket::ConnectingState)
        socket->connectToHost(m_classicIp, 1042);
    return true;
    //return socket->waitForConnected(100);
}
/*
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
            if (!socket->waitForReadyRead(100))
                break;
        }
    toret.truncate(toret.size() - 4);
    return toret;
}*/

void   StuffClient::executeCommand(QByteArray cmd)
{
    state = ClientState::DoingCommand;
    m_commandDatas.clear();
    sDebug() << "Executing command" << cmd;
    writeSocket("CMD " + cmd + "\n");
}

void StuffClient::detachedCommand(QByteArray cmd)
{
    sDebug() << "Running a detached command " << cmd;
    writeSocket("CMD_DETACHED " + cmd + "\n");
}

void StuffClient::streamFile(QByteArray filePath)
{
    state = StreamingFile;
    writeSocket("STREAM_FILE " + filePath + "\n");

}

// Savestate Files are like 500Kbytes, should not take that long

void StuffClient::sendFile(QByteArray filePath, QByteArray data)
{
    writeSocket("PUT_FILE " + QByteArray::number(data.size()) + " " + filePath + "\n");
    writeSocket(data);
    socket->waitForBytesWritten();
}

// This return 10 bytes of size then the data;

void    StuffClient::getFile(QByteArray filePath)
{
    state = ClientState::GettingFile;
    writeSocket("GET_FILE " + filePath + "\n");
}

void StuffClient::close()
{
    socket->disconnect();
    socket->waitForDisconnected(100);
}

QByteArray StuffClient::commandDatas() const
{
    return m_commandDatas;
}

void StuffClient::onReadyRead()
{
    sDebug() << "Ready read" << socket->bytesAvailable();
    switch (state)
    {
    case ClientState::StreamingFile:
    {
        emit newFileData(socket->readAll());
        break;
    }
    case ClientState::DoingCommand:
    {
        m_commandDatas.append(socket->readAll());
        sDebug() << m_commandDatas << m_commandDatas.right(4) << QByteArray(4, 0);
        if (m_commandDatas.right(4) == QByteArray(4, 0))
        {
            sDebug() << "Command finished";
            m_commandDatas.truncate(m_commandDatas.size() - 4);
            emit commandFinished(true);
            return ;
        }
        if (m_commandDatas.right(9) == QByteArray("\0\0ERROR\0\0", 9))
        {
            sDebug() << "Command error";
            m_commandDatas.clear();
            emit commandFinished(false);
        }
        break;
    }
    case ClientState::GettingFile:
    {
        QByteArray socketData = socket->readAll();
        if (fileSizeGot == false)
        {
            fileSizeData.append(socketData);
            if (fileSizeData.size() >= 10)
            {
                fileSizeGot = true;
                off_t size = 0;
                for (unsigned int i = 0; i < 10; i++)
                {
                    size |= fileSizeData.at(i) << 8 * (9 - i);
                }
                emit receivedFileSize(size);
                if (fileSizeData.size() > 10)
                    emit newFileData(fileSizeData.right(10));
            }
        } else {
            emit newFileData(socketData);
        }
        break;
    }
    }
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
