#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QCloseEvent>
#include <QSystemTrayIcon>

class QLabel;
class QLineEdit;
class QPushButton;
class QNetworkRequest;
class QNetworkReply;
class QMenu;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    //void httpFinished();
    void handleTimeout();
    void onSystemTrayIconClicked(QSystemTrayIcon::ActivationReason reason);
    void on_actionQuit_triggered();
    void on_pbSaveConfig_clicked();
    void on_pbClear_clicked();

private:
    void initializeTrayIcon();
    void closeEvent(QCloseEvent *event);
    void writeConfigSettings();
    void readConfigSettings();
    void logMsg(QString msg);
    QString queryPublicIp();
    void updateCloudXnsDns();

    void dnspod_init();
    void dnspod_getDomainList();
    void dnspod_getDomainInfo();
    void dnspod_getRecordList();
    void dnspod_updateDns();


    Ui::MainWindow *ui;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayIconMenu;
    bool m_isQuit;
    QTimer *m_timer;
    QNetworkAccessManager m_qnam;
    QString m_domain, m_publicIp;
    QString m_apiKey, m_secretKey;
    QString m_apiToken;
    QStringList m_dnspodDomainIdList;
    QStringList m_dnspodDomainList;
    QString m_dnspodDomainId;
    QString m_dnspodRecordId;
    QString m_dnspodRecordLineId;
};

#endif // MAINWINDOW_H
