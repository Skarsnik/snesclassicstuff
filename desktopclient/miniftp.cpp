#include "miniftp.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QEventLoop>
#include <QNetworkReply>

Q_LOGGING_CATEGORY(log_miniFTP, "MiniFTP")

#define sDebug() qCDebug(log_miniFTP())

MiniFtp::MiniFtp(QObject *parent) : QObject(parent)
{
    qnam = new QNetworkAccessManager();
    connect(qnam, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(qnam, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), this, SLOT(authenticate(QNetworkReply*, QAuthenticator*)));
}

QByteArray MiniFtp::get(QString file)
{
    sDebug() << "Get : " << QString(HAKCHI2URL) + "/" + file;
    qnam->get(QNetworkRequest(QString(HAKCHI2URL) + "/" + file));
    QEventLoop  loop;
    QObject::connect(this, SIGNAL(dataReceived()), &loop, SLOT(quit()));
    loop.exec();
    return lastDataReceived;
}

void MiniFtp::put(QString dest, QByteArray &data)
{
    sDebug() << "Put : " << data.size() << " to " << QString(HAKCHI2URL) + "/" + dest;
    QNetworkReply* reply = qnam->put(QNetworkRequest(QString(HAKCHI2URL) + "/" + dest), data);
    QEventLoop  loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    if (reply->error() != QNetworkReply::NoError)
    {
        sDebug() << "Reply error : " << reply->error();
        emit error();
    }
}

void MiniFtp::authenticate(QNetworkReply *reply, QAuthenticator *auth)
{
    Q_UNUSED(reply)
    qDebug() << "Authentificate";
    auth->setUser("root");
    auth->setPassword("clover");
}

void MiniFtp::replyFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        lastDataReceived = reply->readAll();
        emit dataReceived();
    } else
    {
        sDebug() << "Reply error : " << reply->error();
        emit error();
    }
}
