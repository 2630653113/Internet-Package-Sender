#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pcap.h"
#include "QMessageBox"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /* 获取本机设备列表 */
    if (pcap_findalldevs_ex((char*)PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
    {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }
    /*在下拉框里写上本机的设备*/
    QStringList list;
    for (d = alldevs; d!=NULL ; d = d->next)
    {
        QString temp = (QString)(d->name)+" "+(QString)(d->description);
        list << temp;
    }
    ui->comboBox_netcard->addItems(list);

    connect(ui->send,  SIGNAL(clicked()), this, SLOT(send_clicked()));
}

//计算检验和
unsigned short CheckSum(unsigned short * buffer, int size)
{
    unsigned long   cksum = 0;
    while (size > 1)
    {
        cksum += *buffer++;
        size -= sizeof(unsigned short);
    }
    if (size)
    {
        cksum += *(unsigned char *)buffer;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (unsigned short)(~cksum);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::send_clicked()
{
    int netnum = ui->comboBox_netcard->currentIndex()+1;
    /* 跳转到选中的适配器 */
    int i;   //for循环变量
    for (d = alldevs, i = 0; i < netnum - 1; d = d->next, i++);
    /* 打开设备 */
    if ((adhandle = pcap_open(d->name,          // 设备名
        65536,            // 65535保证能捕获到不同数据链路层上的每个数据包的全部内容
        PCAP_OPENFLAG_PROMISCUOUS,    // 混杂模式
        1000,             // 读取超时时间
        NULL,             // 远程机器验证
        errbuf            // 错误缓冲池
    )) == NULL)
    {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name);
        /* 释放设备列表 */
        pcap_freealldevs(alldevs);
    }
    /*tcp协议 */
    if (true) {
        string bb = ui->data->text().toStdString();
        char *temp = new char[bb.size() + 1];
        strcpy(temp, bb.c_str());//temp为传输数据  bb.size() 为大小
        EthernetHeader eh;
        IpHeader ih;
        TcpHeader th;
        Psdhdr psh;
        u_char sendbuf[MAX_BUFF_LEN];

        //封包
        //以太网
        eh.DestMAC[0] = 0x8c;
        eh.DestMAC[1] = 0xa6;
        eh.DestMAC[2] = 0xdf;
        eh.DestMAC[3] = 0x94;
        eh.DestMAC[4] = 0x94;
        eh.DestMAC[5] = 0x29;
        eh.SourMAC[0] = 0x80;
        eh.SourMAC[1] = 0x2b;
        eh.SourMAC[2] = 0xf9;
        eh.SourMAC[3] = 0x72;
        eh.SourMAC[4] = 0x5f;
        eh.SourMAC[5] = 0xad;
        eh.EthType = htons(ETH_IP);

        //ip头部
        ih.h_verlen = (4 << 4 | sizeof(ih) / sizeof(unsigned int));
        ih.tos = 0;
        ih.total_len = htons((unsigned short)(sizeof(IpHeader) + sizeof(TcpHeader) + bb.size()));
        ih.ident = 1;
        ih.frag_and_flags = 0x40;
        ih.ttl = ui->ttl->text().toInt();
        ih.proto = PROTO_TCP;
        ih.checksum = 0;
        string te1 = ui->srcIP->text().toStdString();
        string te2 = ui->dstIP->text().toStdString();
        char *te11 = new char[te1.size() + 1];
        char *te22 = new char[te2.size() + 1];
        strcpy(te11, te1.c_str());
        strcpy(te22, te2.c_str());
        ih.sourceIP = inet_addr(te11);
        ih.destIP = inet_addr(te22);
        //tcp伪头部
        psh.saddr = ih.sourceIP;
        psh.daddr = ih.destIP;
        psh.mbz = 0;
        psh.ptcl = ih.proto;
        psh.plen = htons(sizeof(TcpHeader) + bb.size());

        //tcp
        th.th_sport = htons(ui->sport->text().toInt());//源端口
        th.th_dport = htons(ui->dport->text().toInt());//目的端口
        th.th_seq = htonl(ui->seq->text().toInt());//序列号
        th.th_ack = ui->ack->text().toInt();//确认号
        th.th_lenres = (sizeof(TcpHeader) / 4 << 4 | 0);//tcp长度
        th.th_flag = (u_char)ui->sign->text().toInt();//标志
        th.th_win = htons(ui->window->text().toInt());//窗口大小
        th.th_sum = 0;//校验和暂时设为0
        th.th_urp = 0;//偏移

        //计算校验和
        memcpy(sendbuf, &psh, sizeof(Psdhdr));
        memcpy(sendbuf + sizeof(Psdhdr), &th, sizeof(TcpHeader));
        memcpy(sendbuf + sizeof(Psdhdr) + sizeof(TcpHeader), temp, bb.size());
        th.th_sum = CheckSum((unsigned short *)sendbuf, sizeof(Psdhdr) + sizeof(TcpHeader) + bb.size());//tcp
        memset(sendbuf, 0, sizeof(sendbuf));
        memcpy(sendbuf, &ih, sizeof(IpHeader));
        ih.checksum = CheckSum((unsigned short *)sendbuf, sizeof(IpHeader));//ip

        //填充发送缓冲区
        memset(sendbuf, 0, MAX_BUFF_LEN);
        memcpy(sendbuf, (void *)&eh, sizeof(EthernetHeader));
        memcpy(sendbuf + sizeof(EthernetHeader), (void *)&ih, sizeof(IpHeader));
        memcpy(sendbuf + sizeof(EthernetHeader) + sizeof(IpHeader), (void *)&th, sizeof(TcpHeader));
        memcpy(sendbuf + sizeof(EthernetHeader) + sizeof(IpHeader) + sizeof(TcpHeader), temp, bb.size());
        int siz = sizeof(EthernetHeader) + sizeof(IpHeader) + sizeof(TcpHeader) + bb.size();
        //发送
        if (pcap_sendpacket(adhandle, sendbuf, siz) == 0) {
            QMessageBox::information(this,"提示","发送成功");
            //ui->note->append(" TCP Packet send succeed");
        }
        else {
            QMessageBox::information(this,"提示","发送失败");
            //ui->note->append("error");
        }

    }

}


