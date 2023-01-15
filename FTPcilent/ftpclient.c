#include<stdio.h>
#include"ftpclient.h"
char g_recvbuf[1024];//������Ϣ�Ļ�����
char* g_filebuf;//�洢�ļ�����
int g_filesize;//�ļ���С
char g_filename[256];//�ļ���
int main()
{
    Socketinit();
    connectToserver();
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
//�ͻ���socket���ӷ�����
void connectToserver()
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
    serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//��������ip��ַ ipʮ����ת������
    //���ӵ�������
    if (0!=connect(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
    {
        printf("socket connect failed:%d\n", WSAGetLastError());
    }
    downloadFilename(serfd);
    while (processMessage(serfd))
    {
    }
}
//��Ϣ����
bool processMessage(SOCKET s)
{
    recv(s,g_recvbuf,sizeof(struct MsgHeader),0);
    struct MsgHeader* msg = (struct MsgHeader*)g_recvbuf;
    switch (msg->msgID)
    {
    case MSG_OPEN_FAILED:
        downloadFilename(s);
        break;
    case MSG_FILESIZE:
        readyread(s,msg);
        break;
    case MSG_READY_READ:
        writeFile(s,msg);
        break;
    }
    return true;
}
void downloadFilename(SOCKET s)//�����������filename
{
    char filename[1024] = { 0 };
    gets_s(filename, 1023);//��ȡ�����ļ���
    struct MsgHeader file = { .msgID = MSG_FILENAME };
    strcpy(file.fileinfo.filename, filename);
    send(s, (char*)&file, sizeof(struct MsgHeader), 0);
}
void readyread(SOCKET s, struct MsgHeader*pmsg)
{
    printf("MSG_FILESIZE \n");
    //׼���ڴ� pmsg->fileinfo.filesize
    //������������MSG_READY_READ
    strcpy(g_filename ,pmsg->fileinfo.filename);
    g_filesize = pmsg->fileinfo.filesize;
    g_filebuf = calloc(g_filesize + 1, sizeof(char));
    if (g_filebuf == NULL)
    {
        printf("�洢�ļ����ڴ治��\n");
    }
    else
    {
        struct MsgHeader msg = { .msgID = MSG_SENDFILE };
        if (send(s, (char*)&msg, sizeof(struct MsgHeader), 0)==SOCKET_ERROR)
        {
            printf(" MSG_SENDFILE send fail:%d\n",WSAGetLastError());
            return;
        }
    }
    printf("size:%d byte filename:%s \n",pmsg->fileinfo.filesize,pmsg->fileinfo.filename);
    return;
}
bool writeFile(SOCKET s, struct MsgHeader* pmsg)
{
    printf("MSG_READY_READ \n");
    if (g_filebuf == NULL)
    {
        return false;
    }
    int nstart = pmsg->packet.nstart;
    int nsize = pmsg->packet.nsize;
    memcpy(g_filebuf + nstart, pmsg->packet.buf, nsize);
    printf("packet: %d  %d\n",nstart+nsize,g_filesize);
    if (nstart + nsize >= g_filesize)
    {
        FILE* pwrite = fopen(g_filename, "wb");
        if (pwrite == NULL)
        {
            printf("writefile fail\n");
            return false;
        }
        fwrite(g_filebuf, sizeof(char), g_filesize, pwrite);
        fclose(pwrite);
        free(g_filebuf);
        g_filebuf = NULL;
        struct MsgHeader msg; msg.msgID = MSG_SUCCESSED;
        send(s, (char*)&msg, sizeof(struct MsgHeader), 0);
    }
    return true;
}