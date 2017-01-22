#ifndef CRYPTO_DEMO_APPSTRUCT_H
#define CRYPTO_DEMO_APPSTRUCT_H

enum  crypto_demo_type
{
	DTYPE_CRYPTO_DEMO=0x1001
};
enum crypto_demo_subtype
{
	SUBTYPE_LOGIN_INFO=0x01,
	SUBTYPE_VERIFY_RETURN,
	SUBTYPE_CHAT_MSG	
};

struct login_info
{
        char user[DIGEST_SIZE];    //用户名称
        char passwd[DIGEST_SIZE];  //用户口令
        char nonce[DIGEST_SIZE];   //随机数 
} __attribute__((packed));

struct verify_return
{
        int retval;    //返回值，-1为不存在用户，0为口令不正确，1为成功
        int ret_data_size;   //返回信息的大小
        BYTE * ret_data;   //返回信息的内容
        char nonce[DIGEST_SIZE];  //随机值
} __attribute__((packed));

struct session_msg{
	char sender[DIGEST_SIZE]; //发送者名称
	char receiver[DIGEST_SIZE]; //接收者名称
	long  time;               //消息发送时间 
	char * msg;               //消息内容，以’\0’结尾 
}__attribute__((packed));

#endif
