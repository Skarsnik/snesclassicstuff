#ifndef MINIFTP_H
#define MINIFTP_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QAuthenticator>

#define HAKCHI2URL "ftp://root:clover@127.0.0.1:1021"

class MiniFtp : public QObject
{
    Q_OBJECT
public:
    explicit MiniFtp(QObject *parent = nullptr);
    QByteArray    get(QString file);
    void          put(QString dest, QByteArray& data);

signals:
    void          error();
    void          dataReceived();

public slots:


private slots:
    void    authenticate(QNetworkReply* reply, QAuthenticator* auth);
    void    replyFinished(QNetworkReply* reply);


private:
    QNetworkAccessManager*  qnam;
    QByteArray              lastDataReceived;
};

#endif // MINIFTP_H
