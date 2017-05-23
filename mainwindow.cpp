#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDebug>

static const char g_ipServerUrl[] = "http://greak.net/ip";
static const char g_cloudXnsUrl[] = "http://www.cloudxns.net/api2/ddns";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_domain = "f0ghua.tk";
    m_apiKey = "8c51818384738f511d2619af665a7891";
    m_secretKey = "e0ca26f89f1cb174";

    m_timer = new QTimer(this);
    m_timer->setInterval(600*1000);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    m_timer->start();

    handleTimeout();
}

MainWindow::~MainWindow()
{
    delete ui;
    m_timer->stop();
}

void MainWindow::handleTimeout()
{
    m_publicIp = queryPublicIp();
    updateCloudXnsDns();
}

QString MainWindow::queryPublicIp()
{
    QEventLoop loop;
    QNetworkRequest request;
    QString ipStr;

    QString url = g_ipServerUrl;
    request.setUrl(url);
    QNetworkReply *reply = m_qnam.get(request);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->error()) {
        QByteArray data = reply->readAll();
        ipStr = QString(data).simplified();
    }
#ifndef F_NO_DEBUG
    qDebug() << tr("get ip addr: %1").arg(ipStr);
#endif
    return ipStr;
}

void MainWindow::updateCloudXnsDns()
{
    QEventLoop loop;
    QNetworkRequest request;
    QString ipStr;
    QString url = g_cloudXnsUrl;


    QByteArray postData =
            QString("{\"domain\":\"%1\",\"ip\":\"%2\",\"line_id\":\"1\"}").arg(m_domain).arg(m_publicIp).toLatin1();
#ifndef F_NO_DEBUG
    qDebug() << tr("postData: %1").arg(QString(postData));
#endif
    QDateTime time = QDateTime::currentDateTime();
    QString dateTime = QLocale( QLocale::C ).toString(time, "ddd,d MMMM yyyy hh:mm:ss +0800");
#ifndef F_NO_DEBUG
    qDebug() << tr("date: %1").arg(dateTime);
#endif

    QString macRaw = m_apiKey + url + postData + dateTime + m_secretKey;
    QString md5;
    QByteArray ba;
    ba = QCryptographicHash::hash(macRaw.toLatin1(), QCryptographicHash::Md5);
    md5.append(ba.toHex());
#ifndef F_NO_DEBUG
    qDebug() << tr("md5: %1").arg(md5);
#endif

    //return;

    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("API-KEY", m_apiKey.toLatin1());
    request.setRawHeader("API-REQUEST-DATE", dateTime.toLatin1());
    request.setRawHeader("API-HMAC", md5.toLatin1());
    request.setRawHeader("API-FORMAT", "json");
    QNetworkReply *reply = m_qnam.post(request, postData);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        ipStr = QString(data); //.simplified();
    } else {
#ifndef F_NO_DEBUG
        qDebug() << "error:" << reply->errorString();
#endif
    }
#ifndef F_NO_DEBUG
    qDebug() << tr("get ip addr: %1").arg(ipStr);
#endif

}
