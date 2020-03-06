#include "client.h"
#include "ui_client.h"
#include<QTcpSocket>
#include<QDebug>
#include<QMessageBox>

Client::Client(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Client)
{
    ui->setupUi(this);
    setFixedSize(400,190);

    totalBytes=0;
    bytesReceived=0;
    fileNameSize=0;

    tClnt=new QTcpSocket(this);
    tPort=5555;
    connect(tClnt,SIGNAL(readyRead()),this,SLOT(readMsg()));
    connect(tClnt,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayErr(QAbstractSocket::SocketError)));
}

Client::~Client()
{
    delete ui;
}

void Client::displayErr(QAbstractSocket::SocketError sockErr)//传输发生错误会由槽函数处理
{
    switch (sockErr) {
    case QAbstractSocket::RemoteHostClosedError: break;
    default:qDebug()<<tClnt->errorString();
    }
}

void Client::newConn()
{
    blockSize=0;
    tClnt->abort();
    tClnt->connectToHost(hostAddr,tPort);
    time.start();
}

void Client::readMsg()
{
    QDataStream in (tClnt);
    in.setVersion(QDataStream::Qt_4_7);

    float useTime=time.elapsed();

    if(bytesReceived<=sizeof(qint64)*2){
        if((tClnt->bytesAvailable()>=sizeof(qint64)*2)&&(fileNameSize==0))
        {
            in>>totalBytes>>fileNameSize;
            bytesReceived+=sizeof(qint64)*2;
        }
        if((tClnt->bytesAvailable()>=fileNameSize)&&(fileNameSize!=0))
        {
            in>>fileName;
            bytesReceived+=fileNameSize;

            if(!locFile->open(QFile::WriteOnly))
            {
                QMessageBox::warning(this,tr("应用程序"),tr("无法读取文件%1:\n%2.").arg(fileName).arg(locFile->errorString()));
                return;
            }
        }
        else {
            return;
        }
    }
    if(bytesReceived<totalBytes){
        bytesReceived+=tClnt->bytesAvailable();
        inBlock=tClnt->readAll();
        locFile->write(inBlock);
        inBlock.resize(0);
    }
    ui->progressBar->setMaximum(totalBytes);
    ui->progressBar->setValue(bytesReceived);
    double speed=bytesReceived/useTime;
    ui->cStatusLbl->setText(tr("已发送%1MB（%2MB/s）\n共%3MB 已用时：%4秒\n 估计剩余时间：%5秒")
                            .arg(bytesReceived/(1024*1024))
                            .arg(speed*1000/(1024*1024),0,'f',2)
                            .arg(totalBytes/(1024*1024))
                            .arg(useTime/1000,0,'f',0)
                            .arg(totalBytes/speed/1000-useTime/1000,0,'f',0));

    if(bytesReceived==totalBytes){//关闭顺序为：文档->socket
        locFile->close();
        tClnt->close();
        ui->cStatusLbl->setText(tr("接收文件%1完毕").arg(fileName));
    }
}

void Client::on_cCancleBtn_clicked()
{
    tClnt->abort();
    if(locFile->isOpen())
        locFile->close();
}
void Client::on_cCloseBtn_clicked()
{
    tClnt->abort();
    if(locFile->isOpen())
        locFile->close();
    close();//关闭对话框
}
void Client::closeEvent(QCloseEvent *)
{
    on_cCloseBtn_clicked();
}
void Client::setFileName(QString name)//从Widget类中获取文件保存路径
{
    locFile=new QFile(name);
}
void Client::setHostAddr(QHostAddress addr)//从Widget类中获取发送端ip地址，建立连接
{

    hostAddr=addr;
    newConn();
}
