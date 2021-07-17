#ifndef SERVER_H
#define SERVER_H

#include <QDialog>

#include <QTcpServer>

class QTcpSocket;
class QFile;

namespace Ui {
class Server;
}

class Server : public QWidget
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = nullptr);
    ~Server();

private slots:
    void start();
    void stop();
    void acceptConnection();
    void updateServerProgress();
    void displayError(QAbstractSocket::SocketError socketError);

    void on_startButton_clicked();
    void on_stopButton_clicked();

private:
    void appendLog(const QString& text);

    Ui::Server *ui;

    QTcpServer tcpServer;
    QTcpSocket *tcpServerConnection = nullptr;
    qint64 totalBytes;
    qint64 bytesReceived;
    qint64 fileNameSize;
    QString fileName;
    QFile *localFile = nullptr;
    QByteArray inBlock;
};

#endif // SERVER_H
