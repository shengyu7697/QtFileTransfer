#include "server.h"
#include "ui_server.h"
#include <QtNetwork>
#include <QMessageBox>
#include <QFileDialog>

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
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->chooseDownloadDirectoryButton->setEnabled(true);
    ui->downloadLocationLineEdit->setEnabled(true);

    QString ipAddr = getIpAddr();
    appendLog(QStringLiteral("本機 ip: %1\n按 start 開始監聽").arg(ipAddr));

    downloadLocation = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QDir::separator();
    ui->downloadLocationLineEdit->setText(downloadLocation);
}

Server::~Server()
{
    delete ui;
}

void Server::start()
{
    if (!tcpServer.listen(QHostAddress::Any, ui->portLineEdit->text().toInt())) {
        qDebug() << tcpServer.errorString();
        appendLog(tcpServer.errorString());
        QMessageBox msgBox;
        msgBox.setText(tcpServer.errorString());
        msgBox.exec();
        return;
    }
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(true);
    ui->chooseDownloadDirectoryButton->setEnabled(false);
    ui->downloadLocationLineEdit->setEnabled(false);
    totalBytes = 0;
    bytesReceived = 0;
    fileNameSize = 0;
    appendLog(QStringLiteral("監聽"));
    ui->serverProgressBar->reset();
}

void Server::stop()
{
    if (tcpServerConnection != nullptr) {
        tcpServerConnection->close();
    }
    tcpServer.close();
    ui->serverProgressBar->reset();
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->chooseDownloadDirectoryButton->setEnabled(true);
    ui->downloadLocationLineEdit->setEnabled(true);
    appendLog(QStringLiteral("停止傳輸"));
}

void Server::acceptConnection()
{
    tcpServerConnection = tcpServer.nextPendingConnection();
    connect(tcpServerConnection, SIGNAL(readyRead()),
            this, SLOT(updateServerProgress()));
    connect(tcpServerConnection, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    appendLog(QStringLiteral("接受連接"));
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
            appendLog(QStringLiteral("接收文件 %1 ...").arg(fileName));
            bytesReceived += fileNameSize;
            appendLog(QStringLiteral("開檔 %1 ...").arg(downloadLocation + fileName));
            localFile = new QFile(downloadLocation + fileName);
            if (!localFile->open(QFile::WriteOnly)) {
                qDebug() << "server: open file error!";
                appendLog(QStringLiteral("server: open file error!"));
                return;
            }
        } else {
            qDebug() << "server: unkonwn error!";
            appendLog(QStringLiteral("server: unkonwn error!"));
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
        appendLog(QStringLiteral("接收文件 %1 成功！").arg(fileName));
        appendLog(QStringLiteral("儲存位置 %1！").arg(downloadLocation + fileName));

        // 完成時，繼續自動 start
        start();
    }
}

void Server::displayError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "Error: " << tcpServerConnection->errorString();
    appendLog("Error: " + tcpServerConnection->errorString());
    stop();
}

void Server::on_startButton_clicked()
{
    start();
}

void Server::on_stopButton_clicked()
{
    stop();
}

void Server::on_chooseDownloadDirectoryButton_clicked()
{
    appendLog(QStringLiteral("開啟儲存資料夾..."));
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    downloadLocation = dir + QDir::separator();
    ui->downloadLocationLineEdit->setText(downloadLocation);
    appendLog(QStringLiteral("儲存資料夾 %1").arg(downloadLocation));
}

void Server::appendLog(const QString& text)
{
    ui->textBrowser->append(text);
}
