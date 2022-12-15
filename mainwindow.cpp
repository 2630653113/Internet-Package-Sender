#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pcap.h"
#include "QMessageBox"
#include <QButtonGroup>

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

    //设置选择协议的按钮组
    QButtonGroup *choose_protocol = new QButtonGroup(this);
    choose_protocol->addButton(ui->tcp);
    choose_protocol->addButton(ui->udp);
    choose_protocol->addButton(ui->icmp);
    choose_protocol->addButton(ui->ip);
    choose_protocol->addButton(ui->arp);

    //设置选择特定协议后，不需要输入的内容项
    connect(ui->tcp,&QRadioButton::clicked,[this]()
    {
        ui->srcMAC->clear();
        ui->srcMAC->setPlaceholderText("此项无需填写");
        ui->dstMAC->clear();
        ui->dstMAC->setPlaceholderText("此项无需填写");

        ui->ttl->setPlaceholderText("");
        ui->sign->setPlaceholderText("");
        ui->seq->setPlaceholderText("");
        ui->ack->setPlaceholderText("");
        ui->window->setPlaceholderText("");
        ui->sport->setPlaceholderText("");
        ui->dport->setPlaceholderText("");
        ui->srcIP->setPlaceholderText("");
        ui->srcIP->setPlaceholderText("");
    });
    connect(ui->udp,&QRadioButton::clicked,[this]()
    {
        ui->srcMAC->clear();
        ui->srcMAC->setPlaceholderText("此项无需填写");
        ui->dstMAC->clear();
        ui->dstMAC->setPlaceholderText("此项无需填写");
        ui->ttl->clear();
        ui->ttl->setPlaceholderText("128");
        ui->sign->clear();
        ui->sign->setPlaceholderText("此项无需填写");
        ui->seq->clear();
        ui->seq->setPlaceholderText("此项无需填写");
        ui->ack->clear();
        ui->ack->setPlaceholderText("此项无需填写");
        ui->window->clear();
        ui->window->setPlaceholderText("此项无需填写");

        ui->sport->setPlaceholderText("");
        ui->dport->setPlaceholderText("");
        ui->srcIP->setPlaceholderText("");
        ui->srcIP->setPlaceholderText("");
    });
    connect(ui->icmp,&QRadioButton::clicked,[this]()
    {
        ui->srcMAC->setPlaceholderText("此项无需填写");
        ui->dstMAC->setPlaceholderText("此项无需填写");
    });
    connect(ui->ip,&QRadioButton::clicked,[this]()
    {
        ui->srcMAC->setPlaceholderText("此项无需填写");
        ui->dstMAC->setPlaceholderText("此项无需填写");
    });
    connect(ui->arp,&QRadioButton::clicked,[this]()
    {
        ui->sport->clear();
        ui->sport->setPlaceholderText("此项无需填写");
        ui->dport->clear();
        ui->dport->setPlaceholderText("此项无需填写");
        ui->ttl->clear();
        ui->ttl->setPlaceholderText("此项无需填写");
        ui->sign->clear();
        ui->sign->setPlaceholderText("此项无需填写");
        ui->seq->clear();
        ui->seq->setPlaceholderText("此项无需填写");
        ui->ack->clear();
        ui->ack->setPlaceholderText("此项无需填写");
        ui->window->clear();
        ui->window->setPlaceholderText("此项无需填写");

        ui->srcMAC->setPlaceholderText("");
        ui->dstMAC->setPlaceholderText("");
        ui->srcIP->setPlaceholderText("");
        ui->srcIP->setPlaceholderText("");

    });

    //设置发送函数
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

    /*TCP协议 */
    if (ui->tcp->isChecked()) {
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
        eh.DestMAC[0] = 0x9c;
        eh.DestMAC[1] = 0x1d;
        eh.DestMAC[2] = 0x36;
        eh.DestMAC[3] = 0xec;
        eh.DestMAC[4] = 0x8b;
        eh.DestMAC[5] = 0xc2;
        eh.SourMAC[0] = 0x8c;
        eh.SourMAC[1] = 0xc6;
        eh.SourMAC[2] = 0x81;
        eh.SourMAC[3] = 0x96;
        eh.SourMAC[4] = 0x51;
        eh.SourMAC[5] = 0x09;
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
            ui->terminal->setText("TCP报文发送成功！");
        }
        else {
            QMessageBox::information(this,"提示","发送失败");
            ui->terminal->setText("TCP报文发送失败！");
        }
    }

    /*UDP协议 */
    else if (ui->udp->isChecked())
    {
        u_char sendbuf[MAX_BUFF_LEN];
        EthernetHeader eh;
        IpHeader ih;
        UdpHeader uh;
        Psdhdr psh;
        string bb = ui->data->text().toStdString();
        char *temp = new char[bb.size() + 1];
        strcpy(temp, bb.c_str());//temp为传输数据
        //以太网
        eh.EthType = htons(ETH_IP);
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

        //ip
        ih.h_verlen = (4 << 4 | sizeof(ih) / sizeof(unsigned int));//ip数据头总长度除以4
        ih.tos = 0;
        ih.total_len = htons((unsigned short)(sizeof(IpHeader)+sizeof(UdpHeader)+bb.size()));//从ip一直到包末尾的总长度
        ih.ident = htons(1);
        ih.frag_and_flags = htons(0);
        //ih.ttl = ui->ttl->text().toInt();
        ih.ttl = 128;
        ih.proto = PROTO_UDP;
        ih.checksum = 0;
        string te1 = ui->srcIP->text().toStdString();
        string te2 = ui->dstIP->text().toStdString();
        char *te11 = new char[te1.size() + 1];
        char *te22 = new char[te2.size() + 1];
        strcpy(te11, te1.c_str());
        strcpy(te22, te2.c_str());
        ih.sourceIP = inet_addr(te11);
        ih.destIP = inet_addr(te22);


        //udp
        uh.sport = htons(ui->sport->text().toInt());
        uh.dport = htons(ui->dport->text().toInt());
        uh.check = 0;
        uh.len = htons(sizeof(UdpHeader)+bb.size());


        //udp伪头部
        psh.daddr = ih.destIP;
        psh.saddr = ih.sourceIP;
        psh.ptcl = PROTO_UDP;
        psh.mbz = 0;
        psh.plen = htons(sizeof(UdpHeader) + bb.size());

        //计算校验和
        memcpy(sendbuf, &psh, sizeof(psh));
        memcpy(sendbuf + sizeof(psh), &uh, sizeof(uh));
        memcpy(sendbuf + sizeof(psh) + sizeof(uh), temp, bb.size());
        uh.check = CheckSum((unsigned short *)sendbuf, sizeof(psh) + sizeof(uh)+bb.size());
        memset(sendbuf, 0, sizeof(sendbuf));
        memcpy(sendbuf, &ih, sizeof(ih));
        ih.checksum = CheckSum((unsigned short *)sendbuf, sizeof(ih));

        //封包
        memset(sendbuf, 0, MAX_BUFF_LEN);
        memcpy(sendbuf, (void *)&eh, sizeof(EthernetHeader));
        memcpy(sendbuf + sizeof(EthernetHeader), (void *)&ih, sizeof(IpHeader));
        memcpy(sendbuf + sizeof(EthernetHeader) + sizeof(IpHeader), (void *)&uh, sizeof(UdpHeader));
        memcpy(sendbuf + sizeof(EthernetHeader) + sizeof(IpHeader)+sizeof(UdpHeader), temp, bb.size());
        int siz = sizeof(EthernetHeader) + sizeof(IpHeader) + sizeof(UdpHeader) + bb.size();
        //发送
        if (pcap_sendpacket(adhandle, sendbuf, siz) == 0) {
            QMessageBox::information(this,"提示","发送成功");
            ui->terminal->setText("UDP报文发送成功！");
        }
        else {
            QMessageBox::information(this,"提示","发送失败");
            ui->terminal->setText("UDP报文发送失败！");
        }
    }

    /*ICMP协议 */
    else if (ui->icmp->isChecked())
    {

    }

    /*IP协议 */
    else if (ui->ip->isChecked())
    {

    }

    /*ARP协议 */
    else if (ui->arp->isChecked())
    {
        unsigned char sendbuf[42]; //arp包结构大小，42个字节

        unsigned char ip[4] = { 0x01,0x02,0x03,0x04 };
        EthernetHeader eh;
        ArpHeader ah;
        //赋值MAC地址
        memset(eh.DestMAC, 0xff, 6);   //以太网首部目的MAC地址，全为广播地址
        eh.SourMAC[0] = 0x80;
        eh.SourMAC[1] = 0x2b;
        eh.SourMAC[2] = 0xf9;
        eh.SourMAC[3] = 0x72;
        eh.SourMAC[4] = 0x5f;
        eh.SourMAC[5] = 0xad;
        memcpy(ah.smac, eh.SourMAC, 6);   //ARP字段源MAC地址
        memset(ah.dmac, 0xff, 6);   //ARP字段目的MAC地址
        QString te1 = ui->srcIP->text();
        QString te2 = ui->dstIP->text();;
        int ah1 = stol(te1.section('.', 0, 0).toStdString());
        int ah2 = stol(te1.section('.', 1, 1).toStdString());
        int ah3 = stol(te1.section('.', 2, 2).toStdString());
        int ah4 = stol(te1.section('.', 3, 3).toStdString());
        int ah5 = stol(te2.section('.', 0, 0).toStdString());
        int ah6 = stol(te2.section('.', 1, 1).toStdString());
        int ah7 = stol(te2.section('.', 2, 2).toStdString());
        int ah8 = stol(te2.section('.', 3, 3).toStdString());

        ah.sip[0] = ah1;
        ah.sip[1] = ah2;
        ah.sip[2] = ah3;
        ah.sip[3] = ah4;
        ah.dip[0] = ah5;
        ah.dip[1] = ah6;
        ah.dip[2] = ah7;
        ah.dip[3] = ah8;

        eh.EthType = htons(ETH_ARP);   //htons：将主机的无符号短整形数转换成网络字节顺序
        ah.hdType = htons(ARP_HARDWARE);
        ah.proType = htons(ETH_IP);
        ah.hdSize = 6;
        ah.proSize = 4;
        ah.op = htons(ARP_REQUEST);

        //构造一个ARP请求
        memset(sendbuf, 0, sizeof(sendbuf));   //ARP清零
        memcpy(sendbuf, &eh, sizeof(eh));
        memcpy(sendbuf + sizeof(eh), &ah, sizeof(ah));
        //如果发送成功
        if (pcap_sendpacket(adhandle, sendbuf, 42) == 0) {
            QMessageBox::information(this,"提示","发送成功");
            ui->terminal->setText("ARP报文发送成功！");
        }
        else {
            QMessageBox::information(this,"提示","发送成功");
            ui->terminal->setText("ARP报文发送失败！");
        }
    }

    /*提示选择协议 */
    else
    {
        QMessageBox::information(this,"提示","您必须选择一个发送协议");
    }

}



