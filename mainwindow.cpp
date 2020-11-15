#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

static const char g_cloudXnsUrl[] = "http://www.cloudxns.net/api2/ddns";

// reference to https://github.com/anrip/ArDNSPod
//static const char g_loginToken[] = "39298,39e2babdb13b88773f2046ac9116d577";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->grpCloudXNS->hide();

    readConfigSettings();
    /*
    m_domain = "f0ghua.tk";
    m_apiKey = "8c51818384738f511d2619af665a7891";
    m_secretKey = "e0ca26f89f1cb174";
    */
    m_timer = new QTimer(this);
    m_timer->setInterval(m_interval*1000);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    m_timer->start();

    initializeTrayIcon();
    dnspod_init();

    handleTimeout();
}

MainWindow::~MainWindow()
{
    delete ui;
    m_timer->stop();
}

void MainWindow::initializeTrayIcon()
{
    m_isQuit = false;

    m_trayIconMenu = new QMenu(this);
    m_trayIconMenu->addAction(ui->actionQuit);
    m_trayIconMenu->addSeparator();

    m_trayIcon = new QSystemTrayIcon(this);
    QIcon icon(":images/dns_16x16.png");
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip(tr("ddnsUpdate"));
    m_trayIcon->setContextMenu(m_trayIconMenu);
    m_trayIcon->showMessage(tr("ddnsUpdate"), tr("ddnsUpdate"), QSystemTrayIcon::Information, 5000);
    m_trayIcon->show();

    connect(m_trayIcon,
         &QSystemTrayIcon::activated,
         this,
         &MainWindow::onSystemTrayIconClicked);
}

void MainWindow::onSystemTrayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::Trigger:
        break;
    case QSystemTrayIcon::DoubleClick:
        this->setWindowState(Qt::WindowActive);
        this->show();
        break;
    default:
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isQuit) {
        event->accept();
        return;
    }

    if(m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    }
}

void MainWindow::on_actionQuit_triggered()
{
    m_isQuit = true;
    this->hide();
    this->close();
}

void MainWindow::handleTimeout()
{
    m_publicIp = queryPublicIp();
    dnspod_updateDns();
}

void MainWindow::logMsg(QString msg)
{
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString message = QString("[%1] %2").arg(current_date_time).arg(msg);
    ui->pteLog->appendPlainText(message);
}

QString MainWindow::queryPublicIp_ipify()
{
    QEventLoop loop;
    QNetworkRequest request;
    QString ipStr;

    QString url = "https://api.ipify.org/?format=json";
    request.setUrl(url);
    QNetworkReply *reply = m_qnam.get(request);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->error()) {
        QByteArray data = reply->readAll();
        QJsonObject doc = QJsonDocument::fromJson(data).object();
        ipStr = doc.value("ip").toString();
    }

    logMsg(tr("get ip addr: %1").arg(ipStr));
    return ipStr;
}

QString MainWindow::queryPublicIp_greak()
{
    QEventLoop loop;
    QNetworkRequest request;
    QString ipStr;

    QString url = "http://greak.net/ip";
    request.setUrl(url);
    QNetworkReply *reply = m_qnam.get(request);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->error()) {
        QByteArray data = reply->readAll();
        ipStr = QString(data).simplified();
    }

    logMsg(tr("get ip addr: %1").arg(ipStr));
    return ipStr;
}

QString MainWindow::queryPublicIp()
{
    return queryPublicIp_ipify();
}

void MainWindow::dnspod_getDomainList()
{
    QEventLoop loop;
    QNetworkRequest request;
    QByteArray postData;
    QString url = "https://dnsapi.cn/Domain.List";

    postData = QString("login_token=%1&format=json").arg(m_apiToken).toLatin1();
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("UserAgent", "DNSPOD DDNS Client/1.0.0 (fog_hua@126.com)");
    QNetworkReply *reply = m_qnam.post(request, postData);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        qDebug() << QString(data.toStdString().c_str());
        QJsonParseError jsonError;
        auto doc = QJsonDocument::fromJson(data, &jsonError);
        if (!doc.isNull()
                && (jsonError.error == QJsonParseError::NoError)
                && doc.isObject()) {
            auto docObject = doc.object();
            auto domainsArrayValue = docObject.value(QStringLiteral("domains"));
            QJsonArray array = domainsArrayValue.toArray();

            for(int i = 0; i < array.size(); i++) {
                QJsonObject domainObject = array.at(i).toObject();
                QString name = domainObject["name"].toString();
                qDebug() << "domain name =" << name;

            }
        }
    } else {
        logMsg(tr("error: %1").arg(reply->errorString()));
    }
}

void MainWindow::dnspod_init()
{
//    dnspod_getDomainList();
    dnspod_getDomainInfo();
    dnspod_getRecordList();
}

void MainWindow::dnspod_getDomainInfo()
{
    QEventLoop loop;
    QNetworkRequest request;
    QByteArray postData;
    QString url = "https://dnsapi.cn/Domain.Info";

    postData = QString("login_token=%1&format=json&domain=%2").arg(m_apiToken).arg(m_domain).toLatin1();
    logMsg(tr("postData: %1").arg(QString(postData)));

    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("UserAgent", "DNSPOD DDNS Client/1.0.0 (fog_hua@126.com)");
    QNetworkReply *reply = m_qnam.post(request, postData);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QString replayStr = QString(data); //.simplified();
#ifndef F_NO_DEBUG
        qDebug() << replayStr;
#endif
        QRegularExpression re("\"id\":\"(\\d+)\"");
        QRegularExpressionMatch match = re.match(replayStr);
        if (match.hasMatch()) {
            m_dnspodDomainId = match.captured(1);
        }
        logMsg(tr("get domain id: %1").arg(m_dnspodDomainId));

    } else {
        logMsg(tr("error: %1").arg(reply->errorString()));
    }
}

void MainWindow::dnspod_getRecordList()
{
    QEventLoop loop;
    QNetworkRequest request;
    QByteArray postData;
    QString url = "https://dnsapi.cn/Record.List";

    postData = QString("login_token=%1&format=json&domain=%2&sub_domain=").arg(m_apiToken).arg(m_domain).toLatin1();
    logMsg(tr("postData: %1").arg(QString(postData)));

    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("UserAgent", "DNSPOD DDNS Client/1.0.0 (fog_hua@126.com)");
    QNetworkReply *reply = m_qnam.post(request, postData);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        //qDebug() << QString(data.toStdString().c_str());
        QJsonParseError jsonError;
        auto doc = QJsonDocument::fromJson(data, &jsonError);
        if (!doc.isNull()
                && (jsonError.error == QJsonParseError::NoError)
                && doc.isObject()) {
            auto docObject = doc.object();
            auto arrayValue = docObject.value(QStringLiteral("records"));
            QJsonArray array = arrayValue.toArray();

            m_dnspodRecordList.clear();
            for(int i = 0; i < array.size(); i++) {
                QJsonObject object = array.at(i).toObject();
                QString type = object["type"].toString();
                if (type != "A") {
                    continue;
                }

                DnspodRecord record;
                record.name = object["name"].toString();
                record.id = object["id"].toString();
                record.lineId = object["line_id"].toString();
                m_dnspodRecordList.append(record);

                logMsg(tr("get record name = %1, id = %2, line = %3").\
                       arg(record.name).\
                       arg(record.id).\
                       arg(record.lineId));
            }
        }

    } else {
        logMsg(tr("error: %1").arg(reply->errorString()));
    }
}

void MainWindow::dnspod_updateDns()
{
    QEventLoop loop;
    QNetworkRequest request;
    QByteArray postData;
    QString url = "https://dnsapi.cn/Record.Ddns";

    // update all records
    for (const auto &record : m_dnspodRecordList) {
        postData = QString("login_token=%1&format=json&domain_id=%2&record_id=%3&record_line_id=%4&value=%5&sub_domain=%6").\
                arg(m_apiToken).\
                arg(m_dnspodDomainId).\
                arg(record.id).\
                arg(record.lineId).\
                arg(m_publicIp).\
                arg(record.name).\
                toLatin1();
        logMsg(tr("postData: %1").arg(QString(postData)));

        request.setUrl(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        request.setRawHeader("UserAgent", "DNSPOD DDNS Client/1.0.0 (fog_hua@126.com)");
        QNetworkReply *reply = m_qnam.post(request, postData);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QString replayStr = QString(data); //.simplified();
#ifndef F_NO_DEBUG
            qDebug() << replayStr;
#endif
            logMsg(tr("get returned msg: %1").arg(replayStr));

        } else {
            logMsg(tr("error: %1").arg(reply->errorString()));
        }
    }
}

void MainWindow::writeConfigSettings()
{
    QSettings *cfg = new QSettings("config.ini", QSettings::IniFormat);
    cfg->setIniCodec("UTF-8");

    cfg->setValue("interval", m_interval);
    cfg->setValue("domain", ui->leDomain->text());
    cfg->setValue("apikey", ui->leApiKey->text());
    cfg->setValue("secretkey", ui->leSecretKey->text());
    cfg->setValue("apitoken", ui->leApiToken->text());
}

void MainWindow::readConfigSettings()
{
    QSettings *cfg = new QSettings("config.ini", QSettings::IniFormat);
    cfg->setIniCodec("UTF-8");

    m_interval = cfg->value("interval", "1200").toInt();
    m_domain = cfg->value("domain", "").toString();
    ui->leDomain->setText(m_domain);
    m_apiKey = cfg->value("apikey", "").toString();
    ui->leApiKey->setText(m_apiKey);
    m_secretKey = cfg->value("secretkey", "").toString();
    ui->leSecretKey->setText(m_secretKey);
    m_apiToken = cfg->value("apitoken", "").toString();
    ui->leApiToken->setText(m_apiToken);
}

void MainWindow::on_pbSaveConfig_clicked()
{
    writeConfigSettings();
    readConfigSettings();
    dnspod_init();

    handleTimeout();
}

void MainWindow::on_pbClear_clicked()
{
    ui->pteLog->clear();
}
