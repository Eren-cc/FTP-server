#pragma once
#include<WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#include<stdbool.h>//c�������Ϳ�
#define SEVERPORT 1234 //�������˿ں�
#define PACKET_SIZE (1024 - sizeof(int)*3)//���Ĵ�С
enum MSGTAG
{
	MSG_FILENAME = 1,//�ļ���
	MSG_FILESIZE = 2,//�ļ���С��
	MSG_READY_READ = 3,//׼������
	MSG_SENDFILE = 4,//����
	MSG_SUCCESSED = 5,//�������
	MSG_OPEN_FAILED = 6//�ļ���ʧ��
};
#pragma pack(1)
struct MsgHeader //��װ��Ϣͷ
{
	enum MSGTAG msgID;//��ǰ��Ϣ��� 4byte
	union MyUnion
	{
		struct
		{
			char filename[256];//�ļ��� 256 byte
			int filesize;//�ļ���С 4byte
		}fileinfo;//�ļ���Ϣ 
		struct
		{
			int nsize;//���Ĵ�С
			int nstart;//������ʼ
			char buf[PACKET_SIZE];

		}packet;
	};
};
#pragma pack()
//socket��ʼ��api
bool Socketinit();
//socket�ر�
bool Socketclose();
//�����ͻ��˵�TCP����
void connectToserver();
//��Ϣ����
bool processMessage(SOCKET s);
void downloadFilename(SOCKET s);
void readyread(SOCKET s,struct MsgHeader* pmsg);
//д�ļ�
bool writeFile(SOCKET s, struct MsgHeader* pmsg);