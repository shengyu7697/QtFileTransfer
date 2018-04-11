#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>

#include <QAbstractSocket>

class QTcpSocket;
class QFile;

namespace Ui {
class Client;
}

class Client : public QDialog
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = 0);
    ~Client();

private slots:
    void openFile();
    void send();
    void startTransfer();
    void updateClientProgress(qint64);
    void displayError(QAbstractSocket::SocketError);

    void on_openButton_clicked();
    void on_sendButton_clicked();

private:
    Ui::Client *ui;

    QTcpSocket *tcpClient;
    QFile *localFile;
    qint64 totalBytes;
    qint64 bytesWritten;
    qint64 bytesToWrite;
    qint64 payloadSize;
    QString fileName;
    QByteArray outBlock;
};

#endif // CLIENT_H
