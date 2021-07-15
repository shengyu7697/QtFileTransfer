#include "client.h"
#include "ui_client.h"
#include <QtNetwork>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

Client::Client(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Client)
{
    ui->setupUi(this);

    // show git version on window title
    QString version(GIT_VERSION);
    QString winTitle = QStringLiteral("FileTransferClient ") + version;
    this->setWindowTitle(winTitle);

    payloadSize = 64*1024; // 64KB
    totalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;
    tcpClient = new QTcpSocket(this);

    // 當連接服務器成功時，發出connected()信號，開始傳送文件
    connect(tcpClient, SIGNAL(connected()), this, SLOT(startTransfer()));
    connect(tcpClient, SIGNAL(bytesWritten(qint64)),
            this, SLOT(updateClientProgress(qint64)));
    connect(tcpClient, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    ui->sendButton->setEnabled(false);

    setAcceptDrops(true);

    appendLog(QStringLiteral("請將檔案拖曳到此視窗"));
}

Client::~Client()
{
    delete tcpClient;
    delete ui;
}

bool Client::openFile(QString &fileName)
{
    if (fileName.isEmpty())
        return false;

    ui->sendButton->setEnabled(true);
    appendLog(QStringLiteral("準備傳送 %1").arg(fileName));
    return true;
}

void Client::send()
{
    ui->sendButton->setEnabled(false);

    // 初始化已發送字節為0
    bytesWritten = 0;
    appendLog(QStringLiteral("連接中..."));
    tcpClient->connectToHost(ui->hostLineEdit->text(), ui->portLineEdit->text().toInt());
}

void Client::startTransfer()
{
    localFile = new QFile(fileName);
    if (!localFile->open(QFile::ReadOnly)) {
        qDebug() << "client: open file error!";
        appendLog("client: open file error!");
        return;
    }
    // 獲取文件大小
    totalBytes = localFile->size();

    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_4_0);
    QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/')-1);
    // 保留總大小信息空間、文件名大小信息空間，然後輸入文件名
    sendOut << qint64(0) << qint64(0) << currentFileName;

    // 這裡的總大小是總大小信息、文件名大小信息、文件名和實際文件大小的總和
    totalBytes += outBlock.size();
    sendOut.device()->seek(0);

    // 返回outBolock的開始，用實際的大小信息代替兩個qint64(0)空間
    sendOut << totalBytes << qint64((outBlock.size() - sizeof(qint64)*2));

    // 發送完文件頭結構後剩餘數據的大小
    bytesToWrite = totalBytes - tcpClient->write(outBlock);

    appendLog(QStringLiteral("已連接"));
    outBlock.resize(0);
}

void Client::updateClientProgress(qint64 numBytes)
{
    // 已經發送數據的大小
    bytesWritten += (int)numBytes;

    // 如果已經發送了數據
    if (bytesToWrite > 0) {
        // 每次發送payloadSize大小的數據，這裡設置為64KB，如果剩餘的數據不足64KB，
        // 就發送剩餘數據的大小
        outBlock = localFile->read(qMin(bytesToWrite, payloadSize));

        // 發送完一次數據後還剩餘數據的大小
        bytesToWrite -= (int)tcpClient->write(outBlock);

        // 清空發送緩衝區
        outBlock.resize(0);
    } else { // 如果沒有發送任何數據，則關閉文件
        localFile->close();
    }
    // 更新進度條
    ui->clientProgressBar->setMaximum(totalBytes);
    ui->clientProgressBar->setValue(bytesWritten);
    // 如果發送完畢
    if (bytesWritten == totalBytes) {
        appendLog(QStringLiteral("傳送文件 %1 成功").arg(fileName));
        localFile->close();
        tcpClient->close();
    }
}

void Client::displayError(QAbstractSocket::SocketError)
{
    qDebug() << tcpClient->errorString();
    tcpClient->close();
    ui->clientProgressBar->reset();
    appendLog(QStringLiteral("客戶端就緒"));
    ui->sendButton->setEnabled(true);
}

void Client::on_openButton_clicked()
{
    ui->clientProgressBar->reset();
    appendLog(QStringLiteral("狀態：等待打開文件！"));

    fileName = QFileDialog::getOpenFileName(this);
    if (!openFile(fileName)) {
        qDebug() << "Error: openFile failed";
        appendLog("Error: openFile failed");
        return;
    }
}

void Client::on_sendButton_clicked()
{
    send();
}

void Client::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "dragEnterEvent";
    if (event->mimeData()->hasFormat("text/uri-list")) {
        event->acceptProposedAction();
    }
}

void Client::dropEvent(QDropEvent *event)
{
    qDebug() << "dropEvent";

    ui->clientProgressBar->reset();

    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        qDebug() << "Error: urls is empty";
        appendLog("Error: urls is empty");
        return;
    }

    fileName = urls.first().toLocalFile();
    if (!openFile(fileName)) {
        qDebug() << "Error: openFile failed";
        appendLog("Error: openFile failed");
        return;
    }
}

void Client::appendLog(const QString& text)
{
    ui->textBrowser->append(text);
}
