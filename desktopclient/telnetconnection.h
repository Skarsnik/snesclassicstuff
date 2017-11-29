#ifndef TELNETCONNECTION_H
#define TELNETCONNECTION_H

#include <QObject>
#include <QTcpSocket>

class TelnetConnection : public QObject
{
    Q_OBJECT
public:
    enum State {
        Offline,
        Connected,
        Ready,
        WaitingForCommand
    };
    TelnetConnection(const QString& hostname, int port, const QString& user, const QString& password);
    QByteArray syncExecuteCommand(QString cmd);
    State       state();
    void        setOneCommandMode(bool mode);

signals:
    void    connected();
    void    error();
    void    commandReturn(QByteArray);
    void    commandReturnedNewLine(QByteArray);

public slots:
    void    conneect();
    void    executeCommand(QString toSend);
    void    close();

private slots:
    void    onSocketConnected();
    void    onSocketError(QAbstractSocket::SocketError);
    void    onSocketReadReady();
private:
        enum InternalState {
            Init,
            LoginWritten,
            Logged,
            IReady,
            DataWritten,
            WaitingForCmd
        };

        QString m_host;
        int     m_port;
        QString m_user;
        QString m_password;
        QTcpSocket  socket;
        QByteArray lastSent;
        QByteArray readBuffer;
        QByteArray lastCommandReturn;
        State       m_state;
        InternalState m_istate;
        bool    oneCommandMode;


        void    writeToTelnet(QByteArray toWrite);
};

#endif // TELNETCONNECTION_H
