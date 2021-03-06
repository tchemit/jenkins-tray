/*
** stub_Jenkins.cpp
**
**        Created on: Jan 08, 2012
*/

#include <QtGlobal>
#include <QTcpSocket>
#include <QHostAddress>
#include <QRegExp>
#include <QStringList>
#include <QFile>
#include <QDebug>

#include "stub_Jenkins.hh"

#define HTTP_200 "HTTP/1.1 200 Ok\r\n" \
    "Content-Type: text/html; charset=\"utf-8\"\r\n"

#define HTTP_404_MSG "<html><body><h1>Nothing to see here</h1></body></html>\n"
#define HTTP_404_MSG_LEN (sizeof(HTTP_404_MSG) - 1)
#define HTTP_404 "HTTP/1.1 404 Not Found\r\n" \
    "Content-Type: text/html; charset=\"utf-8\"\r\n"

#define HTTP_403_MSG "<html><body><h1>Forbidden</h1></body></html>\n"
#define HTTP_403_MSG_LEN (sizeof(HTTP_403_MSG) - 1)
#define HTTP_403 "HTTP/1.1 403 Forbidden\r\n" \
    "Content-Type: text/html;charset=UTF-8\r\n"

#define HTTP_401_MSG "<html><body><h1>Authorization Required</h1></body></html>\n"
#define HTTP_401_MSG_LEN (sizeof(HTTP_401_MSG) - 1)
#define HTTP_401 "HTTP/1.1 401 Authorization Required\r\n" \
    "Content-Type: text/html; charset=\"utf-8\"\r\n" \
    "WWW-Authenticate: Basic realm=\"Jenkins\"\r\n"

#define HTTP_LENGTH "Content-Length: "

#define CRLF "\r\n"

#define SERVER_PORT 12345

JenkinsServerStub::JenkinsServerStub(QObject *parent) :
    QTcpServer(parent)
{
        listen(QHostAddress::LocalHost, SERVER_PORT);
}

JenkinsServerStub::~JenkinsServerStub()
{
}

void JenkinsServerStub::incomingConnection(int socket)
{
    QTcpSocket* s = new QTcpSocket(this);

    connect(s, SIGNAL(readyRead()), this, SLOT(readClient()));
    connect(s, SIGNAL(disconnected()), this, SLOT(discardClient()));
    s->setSocketDescriptor(socket);
}

QString JenkinsServerStub::baseUri() const
{
    return QString("http://%1:%2").arg(serverAddress().toString()).arg(serverPort());
}

void JenkinsServerStub::readClient()
{
    QTcpSocket* socket = (QTcpSocket*) sender();
    if (socket->canReadLine())
    {
        QStringList tokens = QString(socket->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));

        if (tokens.size() >= 2 && tokens[0] == "GET")
        {
            /* auth required */
            if (isAuth())
            {
                QRegExp authExp("[\\s\r\n]*Authorization:\\s*Basic\\s*"
                        "([0-9A-Za-z+/=]*)[\\s\r\n]*");

                bool auth_found = false;
                while (socket->canReadLine())
                {
                    QString hdrline = socket->readLine();
                    if (auth_found = authExp.exactMatch(hdrline))
                    {
                        if (authExp.cap(1) == m_auth_data)
                            sendFile(tokens[1], *socket);
                        else
                            sendAuthRequired(*socket);
                        break;
                    }
                }

                /* emulate Jenkins behaviour */
                if (!auth_found)
                    sendForbidden(*socket);
            }
            else
                sendFile(tokens[1], *socket);
        }
        socket->close();

        if (socket->state() == QTcpSocket::UnconnectedState)
        {
            delete socket;
        }
    }
}

void JenkinsServerStub::sendFile(const QString &file, QTcpSocket &socket)
{
    QTextStream os(&socket);
    os.setAutoDetectUnicode(true);

    QMap<QString, QString>::const_iterator it =
        m_data.find(file);

    if (it != m_data.end())
    {
        QFile f(it.value());

        if (f.open(QIODevice::ReadOnly))
        {
            QByteArray data(f.readAll());
            os << HTTP_200 << HTTP_LENGTH << data.length() << CRLF CRLF
                << data;
            f.close();
        }
        else
        {
            qWarning("[JenkinsServerStub]: Failed to open file: %s",
                    qPrintable(it.value()));
            QString msg = "<h1>Failed to open \"" + file + "\"</h1>\n";
            os << HTTP_404 << HTTP_LENGTH << msg.length() << CRLF CRLF << msg;
        }
    }
    else
    {
        qWarning("[JenkinsServerStub]: No such file: %s",
                qPrintable(file));
        os << HTTP_404 << HTTP_LENGTH << HTTP_404_MSG_LEN << CRLF CRLF
            << HTTP_404_MSG;
    }
}

void JenkinsServerStub::sendAuthRequired(QTcpSocket &socket)
{
    QTextStream os(&socket);
    os.setAutoDetectUnicode(true);

    qWarning("[JenkinsServerStub]: Authorization Required");
    os << HTTP_401 << HTTP_LENGTH << HTTP_401_MSG_LEN << CRLF CRLF
        << HTTP_401_MSG;
}

void JenkinsServerStub::sendForbidden(QTcpSocket &socket)
{
    QTextStream os(&socket);
    os.setAutoDetectUnicode(true);

    qWarning("[JenkinsServerStub]: Forbidden");

    os << HTTP_403 << HTTP_LENGTH << HTTP_403_MSG_LEN << CRLF CRLF
        << HTTP_403_MSG;
}

void JenkinsServerStub::discardClient()
{
    QTcpSocket* socket = (QTcpSocket*) sender();
    socket->deleteLater();
}

void JenkinsServerStub::setAuth(const QString &user, const QString &pass)
{
    QByteArray auth;
    auth.reserve(user.length() + pass.length() + 2);
    auth += user;
    auth += ":";
    auth += pass;
    m_auth_data = auth.toBase64();
}

bool JenkinsServerStub::isAuth() const
{
    return !m_auth_data.isEmpty();
}

