#include "server.h"
#include "ui_server.h"
#include <QtNetwork>

QString getIpAddr()
{
    QString ipAddr;

    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddr = ipAddressesList.at(i).toString();
            break;
        }
    }

    // if we did not find one, use IPv4 localhost
    if (ipAddr.isEmpty())
        ipAddr = QHostAddress(QHostAddress::LocalHost).toString();

    return ipAddr; // 獲取本機正在使用的IP地址
}

Server::Server(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Server)
{
    ui->setupUi(this);

    // show git version on window title
    QString version(GIT_VERSION);
    QString winTitle = QStringLiteral("FileTransferServer ") + version;
    this->setWindowTitle(winTitle);

    connect(&tcpServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

    QString ipAddr = getIpAddr();
    ui->serverStatusLabel->setText(QStringLiteral("本機 ip: %1\n按 start 開始監聽").arg(ipAddr));
}

Server::~Server()
{
    delete ui;
}

void Server::start()
{
    if (!tcpServer.listen(QHostAddress::Any, ui->portLineEdit->text().toInt())) {
        qDebug() << tcpServer.errorString();
        close();
        return;
    }
    ui->startButton->setEnabled(false);
    totalBytes = 0;
    bytesReceived = 0;
    fileNameSize = 0;
    ui->serverStatusLabel->setText(QStringLiteral("監聽"));
    ui->serverProgressBar->reset();
}

void Server::acceptConnection()
{
    tcpServerConnection = tcpServer.nextPendingConnection();
    connect(tcpServerConnection, SIGNAL(readyRead()),
            this, SLOT(updateServerProgress()));
    connect(tcpServerConnection, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    ui->serverStatusLabel->setText(QStringLiteral("接受連接"));
    // 關閉服務器，不再進行監聽
    tcpServer.close();
}

void Server::updateServerProgress()
{
    QDataStream in(tcpServerConnection);
    in.setVersion(QDataStream::Qt_4_0);

    // 如果接收到的數據小於16個字節，保存到來的文件頭結構
    if (bytesReceived <= sizeof(qint64)*2) {
        if ((tcpServerConnection->bytesAvailable() >= sizeof(qint64)*2) && (fileNameSize == 0)) {
            // 接收數據總大小信息和文件名大小信息
            in >> totalBytes >> fileNameSize;
            bytesReceived += sizeof(qint64) * 2;
        }
        if ((tcpServerConnection->bytesAvailable() >= fileNameSize) && (fileNameSize != 0)) {
            // 接收文件名，並建立文件
            in >> fileName;
            ui->serverStatusLabel->setText(QStringLiteral("接收文件 %1 ...").arg(fileName));
            bytesReceived += fileNameSize;
            localFile = new QFile(fileName);
            if (!localFile->open(QFile::WriteOnly)) {
                qDebug() << "server: open file error!";
                return;
            }
        } else {
            return;
        }
    }
    // 如果接收的數據小於總數據，那麼寫入文件
    if (bytesReceived < totalBytes) {
        bytesReceived += tcpServerConnection->bytesAvailable();
        inBlock = tcpServerConnection->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }
    ui->serverProgressBar->setMaximum(totalBytes);
    ui->serverProgressBar->setValue(bytesReceived);

    // 接收數據完成時
    if (bytesReceived == totalBytes) {
        tcpServerConnection->close();
        localFile->close();
        ui->startButton->setEnabled(true);
        ui->serverStatusLabel->setText(QStringLiteral("接收文件 %1 成功！").arg(fileName));

        // 完成時，是否繼續自動 start
        if (ui->keepStart->isChecked())
            start();
    }
}

void Server::displayError(QAbstractSocket::SocketError socketError)
{
    qDebug() << tcpServerConnection->errorString();
    tcpServerConnection->close();
    ui->serverProgressBar->reset();
    ui->serverStatusLabel->setText(QStringLiteral("服務端就緒"));
    ui->startButton->setEnabled(true);
}

void Server::on_startButton_clicked()
{
    start();
}
