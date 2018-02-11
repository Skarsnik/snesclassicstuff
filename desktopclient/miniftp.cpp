#include "miniftp.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QEventLoop>
#include <QNetworkReply>

Q_LOGGING_CATEGORY(log_miniFTP, "MiniFTP")

#define sDebug() qCDebug(log_miniFTP())

MiniFtp::MiniFtp(QObject *parent) : QObject(parent)
{
    /*qnam = new QNetworkAccessManager();
    connect(qnam, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(qnam, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), this, SLOT(authenticate(QNetworkReply*, QAuthenticator*)));*/
    QObject::connect(&qftp, SIGNAL(readyRead()), this, SLOT(onFtpReadReady()));
    QObject::connect(&qftp, SIGNAL(commandFinished(int,bool)), this, SLOT(onFtpCommandFinished(int, bool)));
    QObject::connect(&qftp, SIGNAL(stateChanged(int)), this, SLOT(onFtpStateChanged(int)));
    m_state = None;
}

void MiniFtp::connect()
{
    if (m_state != QFtp::Unconnected)
        return;
    sDebug() << "Connecting to server";
    qftp.connectToHost("127.0.0.1", 1021);
    qftp.login("root", "clover");
    //qftp.setTransferMode(QFtp::Active);
}

QByteArray MiniFtp::get(QString file)
{
    sDebug() << "Get : " << file;
    /*qnam->get(QNetworkRequest(QString(HAKCHI2URL) + "/" + file));*/
    connect();
    lastGetCmd = qftp.get(file);
    sDebug() << "Get cmd id is : " << lastGetCmd;
    QEventLoop  loop;
    QObject::connect(this, SIGNAL(dataReceived()), &loop, SLOT(quit()));
    loop.exec();
    return lastDataReceived;
}

void MiniFtp::put(QString dest, QByteArray &data)
{
    sDebug() << "Put : " << data.size() << " to " << dest;
    connect();
    lastPutCmd = qftp.put(data, dest);
    sDebug() << "Put cmd id is : " << lastPutCmd;
    QEventLoop  loop;
    QObject::connect(this, SIGNAL(ftpPutFinished()), &loop, SLOT(quit()));
}

MiniFtp::State MiniFtp::state()
{
    return m_state;
}

void MiniFtp::authenticate(QNetworkReply *reply, QAuthenticator *auth)
{
    Q_UNUSED(reply)
    sDebug() << "Authentificate";
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
        sDebug() << reply->rawHeaderList();
        sDebug() << "Reply error : " << reply->error();
        emit error();
    }
}

void MiniFtp::onFtpReadReady()
{
    /*sDebug() << "On FTP read Ready";
    //lastDataReceived = qftp.readAll();
    sDebug() << "Data received" << lastDataReceived.size();
    //emit dataReceived();*/
}

void MiniFtp::onFtpCommandFinished(int id, bool error)
{
    sDebug() << id << error << qftp.errorString();
    if (id == lastGetCmd)
    {
        sDebug() << "We get data or not";
        if (error)
            lastDataReceived.clear();
        else
            lastDataReceived = qftp.readAll();
        emit dataReceived();
    }
    if (id == lastPutCmd)
    {
        if (error)
            sDebug() << "Put error : " << qftp.errorString();
        else
            emit ftpPutFinished();
    }
}

void MiniFtp::onFtpStateChanged(int state)
{
    sDebug() << "State changed to " << state << qftp.errorString();
    if (state == QFtp::LoggedIn)
    {
        sDebug() << "Connected";
        m_state = Connected;
    }
    else
        m_state = None;
}
