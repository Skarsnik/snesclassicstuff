#include "telnetconnection.h"
#include <QDebug>
#include <QEventLoop>

// TELNET STUFF


#define CLOVER_SHELL_PROMPT "root@CLOVER:~# "
#define CLOVER_SHELL_PROMPT_SIZE 15
#define DO 0xfd
#define WONT 0xfc
#define WILL 0xfb
#define DONT 0xfe
#define CMD 0xff
#define CMD_ECHO 1
#define ITPC 244

#define sDebug() qDebug() << debugName

TelnetConnection::TelnetConnection(const QString &hostname, int port, const QString &user, const QString &password) : QObject()
{
    qRegisterMetaType<TelnetConnection::ConnectionError>("TelnetConnection::ConnectionError");
    m_host = hostname;
    m_port = port;
    m_user = user;
    m_password = password;
    m_istate = Init;
    connect(&socket, SIGNAL(readyRead()), this, SLOT(onSocketReadReady()));
    connect(&socket, SIGNAL(connected()), this, SLOT(onSocketConnected()));
    connect(&socket, SIGNAL(disconnected()), this, SLOT(onSocketDisconnected()));
    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    m_state = Offline;
    debugName = "default";
    oneCommandMode = false;
}

QByteArray TelnetConnection::syncExecuteCommand(QString cmd)
{
    if (!(m_state == Ready || m_state == Connected))
        return QByteArray();
    QEventLoop loop;
    executeCommand(cmd);
    connect(this, SIGNAL(commandReturn(QByteArray)), &loop, SLOT(quit()));
    loop.exec();
    return lastCommandReturn;
}

TelnetConnection::State TelnetConnection::state()
{
    return m_state;
}

void TelnetConnection::setOneCommandMode(bool mode)
{
    oneCommandMode = mode;
}

void TelnetConnection::conneect()
{
    socket.connectToHost(m_host, m_port);
}

void TelnetConnection::executeCommand(QString toSend)
{
    //qDebug() << toSend;
    writeToTelnet(toSend.toLatin1() + "\r\n");
    m_state = WaitingForCommand;
}

void TelnetConnection::close()
{
    if (m_state == Ready)
        executeCommand("exit");
    else {
        qDebug() << "Sending interupt";
        char buf[3];
        buf[0] = CMD;
        buf[1] = CMD;
        buf[2] = ITPC;
        socket.write(buf, 3);
    }
    if (m_state != Offline)
        socket.close();
}

void TelnetConnection::onSocketConnected()
{
    //socket.write("");
    m_istate = Init;
}

void TelnetConnection::onSocketError(QAbstractSocket::SocketError error)
{
    qDebug() << socket.errorString();
    if (error == QAbstractSocket::ConnectionRefusedError)
        emit connectionError(TelnetConnection::ConnectionRefused);
}

void TelnetConnection::onSocketDisconnected()
{
    m_state = Offline;
    sDebug() << "Disconnected";
    emit disconnected();
}


#define FUCK_LINE_SIZE 80

void TelnetConnection::onSocketReadReady()
{
  static unsigned int charToCheck = FUCK_LINE_SIZE - CLOVER_SHELL_PROMPT_SIZE;
  static unsigned int nbRM = 0;

  QByteArray data = socket.read(1024);
  //sDebug() << "DATA:" << data;
  //sDebug() << m_state << m_istate;
  if (m_istate == Init)
  {
      if (data.indexOf("Error: NES Mini is offline") != -1)
      {
          emit connectionError(TelnetConnection::SNESNotRunning);
          return;
      }
  }
  if (data.at(0) == (char) 0xFF)
  {
      qDebug() << "Telnet cmd receive, we don't care for now";
      char buf[3];
      buf[0] = CMD;
      buf[1] = WILL;
      buf[2] = 34; // Line mode;
      socket.write(buf,  3);
      return ;
  }
  //qDebug() << "RAW:" << data;
  if (m_istate == LoginWritten && data == CLOVER_SHELL_PROMPT)
  {
      m_istate = Logged;
      m_state = Connected;
      emit connected();
      return ;
  }
  if (data.indexOf("CLOVER login: ") != -1)
  {
      writeToTelnet("root\r\n");
      m_istate = LoginWritten;
      return;
  }
  if (m_istate < Logged)
      return ;
  readBuffer.append(data);
  //qDebug() << m_istate << readBuffer.size();
  // We entered the login
  if (m_istate == LoginWritten)
  {
      if (readBuffer == "root\r\n")
          readBuffer.clear();
      return;
  }
  // A command string has be written, we want to remove the feedback of it
  if (m_istate == DataWritten)
  {
      qDebug() << "DataWritten" << lastSent << lastSent.size() << "===" << readBuffer << readBuffer.size();
      //Bullshit to remove \r\r\n when the cmd sent is more than 80 (including prompt) (fuck you telnet)
      while (readBuffer.size() > charToCheck)
      {
          while (charToCheck < readBuffer.size() && nbRM < 3)
          {
              nbRM++;
              readBuffer.remove(charToCheck, 1);
          }
          if (nbRM == 3)
          {
            charToCheck += FUCK_LINE_SIZE;
            nbRM = 0;
          }
          //qDebug() << readBuffer << nbRM << readBuffer.size() << charToCheck;
      }
      if (readBuffer.indexOf(lastSent) == 0)
      {
          m_istate = WaitingForCmd;
          readBuffer.remove(0, lastSent.size());
          charToCheck = FUCK_LINE_SIZE - CLOVER_SHELL_PROMPT_SIZE;
      }
  }
  // We are waiting for the output of a command
  if (m_istate == WaitingForCmd)
  {
      //qDebug() << "Waiting for command";
      int pos = readBuffer.indexOf(CLOVER_SHELL_PROMPT);
      if (oneCommandMode)
      {
          if (readBuffer.indexOf("\r\n") != -1)
          {
              //qDebug() << "PIKOOOOOOOOOOOOOOOOOOO";
              //qDebug() << readBuffer;
              int rPos = readBuffer.indexOf("\r\n");
              while (rPos != -1)
              {
                //qDebug() << readBuffer.left(rPos);
                emit  commandReturnedNewLine(readBuffer.left(rPos));
                readBuffer.remove(0, rPos + 2);
                rPos = readBuffer.indexOf("\r\n");
              }
              readBuffer.clear();
          }
      }
      if (pos != -1)
      {
        readBuffer.remove(pos, QString(CLOVER_SHELL_PROMPT).size());
        qDebug() << "=======\n" << "Received shell cmd data : " << readBuffer << "\n=======\n";
        m_state = Ready;
        m_istate = IReady;
        lastCommandReturn = readBuffer;
        emit commandReturn(lastCommandReturn);
        readBuffer.clear();
        charToCheck = FUCK_LINE_SIZE - CLOVER_SHELL_PROMPT_SIZE;
      }
  }
}

void TelnetConnection::writeToTelnet(QByteArray toWrite)
{
    if (m_istate == IReady || m_istate == Logged)
        m_istate = DataWritten;
    lastSent = toWrite;
    socket.write(toWrite);
}
