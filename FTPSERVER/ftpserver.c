#include<stdio.h>
#include"ftpserver.h"
char g_recvbuf[1024];
int g_filesize;//�ļ���С
char* g_filebuf;//�洢�ļ�����
int main()
{
    Socketinit();

    listenToclient();

    Socketclose();
 
	return 0;
}
//��ʼ��socket api�⣨windows���в�����
bool Socketinit()
{
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
    {
        printf("WSAStartup failed :%d\n", WSAGetLastError());
        return false;
    }
    return true;
}
//�ر�socket API��
bool Socketclose()
{
    if (WSACleanup() != 0)
    {
        printf("WSACleanup failed :%d\n", WSAGetLastError());
        return false;
    }
    return true;
}
//�����ͻ��˵�TCP����
void listenToclient()
{
    //����socket�׽��ִ�������Ԫ�飩
    SOCKET serfd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serfd == INVALID_SOCKET)
    {
        printf("socket creation failed:%d\n",WSAGetLastError());
        return;
    }
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(SEVERPORT);//�����ֽ���ת�����ֽ���
    serAddr.sin_addr.S_un.S_addr = ADDR_ANY;//���������������� һ������=һ��mac��ַ
    if (bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)) != 0)//�󶨼���ocket�ı��ض�Ԫ��
    {
        printf("socket bind failed:%d\n", WSAGetLastError());
        return;
    }
    if (listen(serfd, 5)!=0)//�Ѽ�����������Ϊ5��
    {
        printf("socket listen failed:%d\n", WSAGetLastError());
        return;
    }
    struct sockaddr_in cliAddr;
    int len = sizeof(cliAddr);
    SOCKET clifd=accept(serfd, (struct sockaddr*)&cliAddr,&len);//acceptȡ�����ֵ�TCP����,���ؽ��������ӵ���socket clifd
    if(clifd == INVALID_SOCKET)
    {
        printf("socket accpet failed:%d\n", WSAGetLastError());
        return;
    }
    
    while (processMessage(clifd))
    {
    }
}
//��Ϣ����
bool processMessage(SOCKET clifd)
{

    int res=recv(clifd,g_recvbuf,1024,0);
    if (res <= 0)
    {
        printf("�ͻ������ߣ�%d", WSAGetLastError());
        return false;
    }
    
    struct  MsgHeader* msg = (struct MsgHeader*)g_recvbuf;//�ֽ���ǿת�ṹ��
    switch (msg->msgID)
    {
        case MSG_FILENAME:
            readfile(clifd, msg);
        break;
        case MSG_SENDFILE:
            sendfile(clifd, msg);
        break;
        case MSG_SUCCESSED:
            printf("MSG_SUCCESSED\n");
        break;
    }
    //printf("%s\n", g_recvbuf);
    return true;
}
/*
* 1.�ͻ������������ļ�=������Ҫ�����ļ������͸�������
* 2.���������տͻ��˷��͹������ļ���-�������ļ����ҵ��ļ������ļ���С���͸��ͻ���
* 3.�ͻ��˽��յ��ļ���С-��׼����ʼ���գ������ڴ� ׼����ɲ����߷��������Խ���
* 4.���������ղ���ʼ����-�������ļ�����
* 5.�ͻ��˿�ʼ�� �����ݲ��洢-��������ɣ����߷������������
* 6.�ر�����
*/
bool readfile(SOCKET clifd, struct MsgHeader* msg)
{
    FILE* pread = fopen(msg->fileinfo.filename, "rb");
    if (pread == NULL)
    {
        printf("�Ҳ�����%s���ļ�", msg->fileinfo.filename);
        struct MsgHeader pmsg;pmsg.msgID = MSG_OPEN_FAILED;
        if (send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
        {
            printf("send failed:%d\n", WSAGetLastError());
        }
        return false;
    }
    //��ȡ�ļ���С
    fseek(pread, 0, SEEK_END);
    g_filesize = ftell(pread);
    fseek(pread, 0, SEEK_SET);
    struct MsgHeader pmsg; pmsg.msgID = MSG_FILESIZE; pmsg.fileinfo.filesize = g_filesize;
    //c:\users\team\desktop\sese.jpg tfname=see text=.jpg
    char tfname[200] = { 0 }, text[100];
    _splitpath(msg->fileinfo.filename,NULL,NULL,tfname,text);//c����·���ָ�
    strcat(tfname,text);
    strcpy(pmsg.fileinfo.filename,tfname);
    printf("readfile filename:%s filesize:%d\n",pmsg.fileinfo.filename,pmsg.fileinfo.filesize);
    send(clifd, (char*)&pmsg, sizeof(struct MsgHeader),0);//���ķ���

    //��ȡ�����ļ����ݵ��ڴ滺����g_filebuf
    g_filebuf = calloc(g_filesize + 1, sizeof(char));
    if (g_filebuf == NULL)
    {
        printf("�洢�ļ����ڴ治��\n");
        return false;
    }
    fread(g_filebuf,sizeof(char),g_filesize,pread);
    g_filebuf[g_filesize] = '\0';//�˾��ѿ��ӷ�ƨ
    fclose(pread);
}
bool sendfile(SOCKET s, struct MsgHeader* msg)
{
    struct MsgHeader pmsg;
    pmsg.msgID = MSG_READY_READ;
    //����һ�δ�����ķְ���
    for (size_t i=0;i<g_filesize;i = i+PACKET_SIZE)
    {
        pmsg.packet.nstart = i;
        //�ж��Ƿ��ļ����һ����
        if (i + PACKET_SIZE+1>g_filesize)
        {
            pmsg.packet.nsize = g_filesize - i;
        }
        else
        {
            pmsg.packet.nsize = PACKET_SIZE;
        }
        memcpy(pmsg.packet.buf, g_filebuf+msg->packet.nstart,msg->packet.nsize);

        if (SOCKET_ERROR == send(s, (char*)&pmsg, sizeof(struct MsgHeader), 0))
        {
            printf("�ļ�����ʧ��:%d\n", WSAGetLastError());
        }
    }
    return true;
}