#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QNetworkAccessManager>

class QLabel;
class QLineEdit;
class QPushButton;
class QNetworkRequest;
class QNetworkReply;

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

private:
    QString queryPublicIp();
    void updateCloudXnsDns();

    Ui::MainWindow *ui;
    QTimer *m_timer;
    QNetworkAccessManager m_qnam;
    QString m_domain, m_publicIp;
    QString m_apiKey, m_secretKey;
};

#endif // MAINWINDOW_H
