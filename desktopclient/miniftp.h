#ifndef MINIFTP_H
#define MINIFTP_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QAuthenticator>
#include "../qtftp/src/qftp/qftp.h"

#define HAKCHI2URL "ftp://root:clover@127.0.0.1:1021"
#define HAKCHI2HOST "127.0.0.1:1021"

class MiniFtp : public QObject
{
    Q_OBJECT
public:
    enum State {
        None,
        Connected
    };
    explicit MiniFtp(QObject *parent = nullptr);
    void          connect();
    QByteArray    get(QString file);
    void          put(QString dest, QByteArray& data);
    void          close();
    State         state();

signals:
    void            error();
    void            dataReceived();
    void            ftpPutFinished();
    void            connected();
    void            disconnected();

public slots:


private slots:
    void    authenticate(QNetworkReply* reply, QAuthenticator* auth);
    void    replyFinished(QNetworkReply* reply);
    void    onFtpReadReady();
    void    onFtpCommandFinished(int id, bool error);
    void    onFtpStateChanged(int);


private:
    QFtp                    qftp;
    QNetworkAccessManager*  qnam;
    QByteArray              lastDataReceived;
    State                   m_state;
    int                     lastGetCmd;
    int                     lastPutCmd;
};

#endif // MINIFTP_H
