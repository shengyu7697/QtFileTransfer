#ifndef SERVER_H
#define SERVER_H

#include <QDialog>

#include <QTcpServer>

class QTcpSocket;
class QFile;

namespace Ui {
class Server;
}

class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = 0);
    ~Server();

private slots:
    void start();
    void acceptConnection();
    void updateServerProgress();
    void displayError(QAbstractSocket::SocketError socketError);

    void on_startButton_clicked();

private:
    Ui::Server *ui;

    QTcpServer tcpServer;
    QTcpSocket *tcpServerConnection;
    qint64 totalBytes;
    qint64 bytesReceived;
    qint64 fileNameSize;
    QString fileName;
    QFile *localFile;
    QByteArray inBlock;
};

#endif // SERVER_H
