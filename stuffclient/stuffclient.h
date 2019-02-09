#ifndef STUFFCLIENT_H
#define STUFFCLIENT_H

#include <QObject>
#include <QTcpSocket>

class StuffClient : public QObject
{
    Q_OBJECT
public:
    explicit StuffClient(QObject *parent = nullptr);
    bool        connect();
    QByteArray  waitForCommand(QByteArray cmd);
    void        detachedCommand(QByteArray cmd);
    void        streamFile(QByteArray filePath);
    void        sendFile(QByteArray filePath, QByteArray data);
    QByteArray  getFile(QByteArray filePath);
    void        close();

signals:
    void        newFileData(QByteArray);
    void        disconnected();
    void        error();
    void        connected();

public slots:
    void        onReadyRead();
    bool        isConnected();

private:
    QTcpSocket*     socket;
    bool            streamMode;
    void writeSocket(QByteArray data);
};

#endif // STUFFCLIENT_H
