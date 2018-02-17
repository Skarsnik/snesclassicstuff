#include "telnetconnection.h"
#include <QDebug>
#include <QLoggingCategory>
#include <QEventLoop>


Q_LOGGING_CATEGORY(log_telnetconnection, "Telnet")

#define sDebug() qCDebug(log_telnetconnection()) << debugName

// TELNET STUFF


#define CLOVER_SHELL_PROMPT "root@CLOVER:~# "
#define CHANGED_PROMPT "KLDFJLD45SDKL@4"
#define CLOVER_SHELL_PROMPT_SIZE 15
#define DO 0xfd
#define WONT 0xfc
#define WILL 0xfb
#define DONT 0xfe
#define CMD 0xff
#define CMD_ECHO 1
#define ITPC 244



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
    sDebug() << "Executing : " << toSend;
    writeToTelnet(toSend.toLatin1() + "\r\n");
    m_state = WaitingForCommand;
}

void TelnetConnection::close()
{
    if (m_state == Ready)
        executeCommand("exit");
    else {
        sDebug() << "Sending interupt";
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
    sDebug() << socket.errorString();
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
  sDebug() << "DATA:" << data;
  sDebug() << m_state << m_istate;
  if (m_istate == Init)
  {
      if (data.indexOf("Error: NES Mini is offline") != -1)
      {
          emit connectionError(TelnetConnection::SNESNotRunning);
          return;
      }
  }
  if (data.at(0) == (char) 0xFF && m_istate < IReady)
  {
      sDebug() << "Telnet cmd receive, we don't care for now";
      sDebug() << "Received : " << data;
      char buf[3];
      buf[0] = CMD;
      buf[1] = WILL;
      buf[2] = 34; // Line mode;
      socket.write(buf,  3);
      return ;
  }
  if (data.indexOf(0xFF) != -1)
  {
      int pos = data.indexOf(0xFF);
      sDebug() << "removing telnet cmd :" << data.at(pos +1);
      if (data.at(pos + 1) == (char) 0xFF)
          data.remove(pos, 3);
      else
          data.remove(pos, 2);
  }
  if (m_istate == PromptChangeWritten)
  {
      readBuffer.append(data);
      if (readBuffer == "PS1='" + QByteArray(CHANGED_PROMPT) + "'\r\n" + QByteArray(CHANGED_PROMPT))
      {
          m_istate = PromptChanged;
          readBuffer.clear();
          m_state = Connected;
          emit connected();
      }
      return ;
  }
  //qDebug() << "RAW:" << data;
  if (m_istate == LoginWritten && data == CLOVER_SHELL_PROMPT)
  {
      m_istate = Logged;
      writeToTelnet("PS1='" + QByteArray(CHANGED_PROMPT) + "'\r\n");
      m_istate = PromptChangeWritten;
      return ;
  }
  if (data.indexOf("CLOVER login: ") != -1)
  {
      writeToTelnet("root\r\n");
      m_istate = LoginWritten;
      return;
  }
  if (m_istate < PromptChanged)
      return ;
  readBuffer.append(data);
  //qDebug() << m_istate << readBuffer.size();
  // We entered the login
  // A command string has be written, we want to remove the feedback of it
  if (m_istate == DataWritten)
  {
      //sDebug() << "DataWritten" << lastSent << lastSent.size() << "===" << readBuffer << readBuffer.size();
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
          //sDebug() << readBuffer << nbRM << readBuffer.size() << charToCheck;
      }
      if (readBuffer.indexOf(lastSent) == 0)
      {
          m_istate = WaitingForCmd;
          readBuffer.remove(0, lastSent.size());
          charToCheck = FUCK_LINE_SIZE - CLOVER_SHELL_PROMPT_SIZE;
      }
  }
  sDebug() << "PIKOOOOOOOOOOO" << WaitingForCmd << m_istate;
  // We are waiting for the output of a command
  if (m_istate == WaitingForCmd)
  {
      sDebug() << "Waiting for command" << readBuffer;
      int pos = readBuffer.indexOf(CHANGED_PROMPT);
      if (oneCommandMode)
      {
          if (readBuffer.indexOf("\r\n") != -1)
          {
              qDebug() << "PIKOOOOOOOOOOOOOOOOOOO";
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
        readBuffer.remove(pos, QString(CHANGED_PROMPT).size());
        sDebug() << "======="; sDebug() << "Received shell cmd data"; sDebug() << readBuffer ; sDebug() << "=======";
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
    if (m_istate == IReady || m_istate == PromptChanged)
        m_istate = DataWritten;
    lastSent = toWrite;
    socket.write(toWrite);
}
