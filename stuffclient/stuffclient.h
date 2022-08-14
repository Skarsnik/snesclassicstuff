#ifndef STUFFCLIENT_H
#define STUFFCLIENT_H

#include <QObject>
#include <QTcpSocket>

class StuffClient : public QObject
{
    Q_OBJECT
public:
    enum ClientState {
        NONE,
        DoingCommand,
        StreamingFile,
        SendingFile,
        GettingFile
    };


    explicit StuffClient(QObject *parent = nullptr);
             StuffClient(QString snesclassicIp, QObject* parent = nullptr);
    bool        connect();
    //QByteArray  waitForCommand(QByteArray cmd);
    void        executeCommand(QByteArray cmd);
    void        detachedCommand(QByteArray cmd);
    void        streamFile(QByteArray filePath);
    void        sendFile(QByteArray filePath, QByteArray data);
    void        getFile(QByteArray filePath);
    void        close();
    QByteArray  commandDatas() const;

signals:
    void        commandFinished(bool success);
    void        newStreamedFileData(QByteArray);
    void        newFileData(QByteArray data);
    void        receivedFileSize(uint size);
    void        disconnected();
    void        error();
    void        connected();

public slots:
    void        onReadyRead();
    bool        isConnected();

private:
    ClientState     state;
    QTcpSocket*     socket;
    bool            streamMode;
    QByteArray      m_commandDatas;
    QString         m_classicIp;
    QByteArray      fileSizeData;
    bool            fileSizeGot;
    void writeSocket(QByteArray data);
};

#endif // STUFFCLIENT_H
