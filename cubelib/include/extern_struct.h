/*************************************************
*       project:        973 trust demo, zhongan secure os 
*                       and trust standard verify
*	name:		extern_struct.h
*	write date:    	2011-08-04
*	auther:    	Hu jun
*       content:        this file describe the module's extern struct 
*       changelog:       
*************************************************/
#ifndef _OS210_EXTERN_STRUCT_H
#define _OS210_EXTERN_STRUCT_H
//#include "data_type.h"
typedef struct tagMAC_Label   //强制访问控制标记
{
   BYTE ConfLevel;         	//保密级别 
   BYTE InteLevel;         //完整级别
   BYTE SecClass[8];        //主/客体范畴
} __attribute__((packed)) MAC_LABEL;

typedef struct tagTrust_Label   //强制访问控制标记
{
   BYTE DomainType;         //保密级别 
   int DomainNo;           //完整级别
   BYTE Flag;        //主/客体范畴
   UINT16 State;
} __attribute__((packed)) TRUST_LABEL;

// subject label 
typedef struct tagSubjectLabel
{
   char * subname;   	  //subject name
   V_String GroupName;    //subject's group name 
   MAC_LABEL MacLabel;    //subject's secure label
// TRUST_LABEL TrustLabel;  // subject's trust label
   BYTE SubType;	    //subject type
   WORD SubID;		    //subject's unique id in local system	
   BYTE Keypart[32];        //subject key part 
} __attribute__((packed)) SUB_LABEL;  

// object label 
typedef struct tagObjectLabel
{
   char * ObjName;   	  //object name
   MAC_LABEL MacLabel;    //object's secure label
// TRUST_LABEL TrustLabel;  // object's trust label
   BYTE ObjType;	    //object type
   WORD MntID;		    //the Mount file system's unique id
   DWORD ObjID;		    //the Object's unique id 	
   BYTE Digest[32];         //object data's digest
   BYTE KeyPart[32];        //object key part
} __attribute__((packed)) OBJ_LABEL;  

// protocol head struct 
typedef struct tagProtocolHead 
{
  	 UINT32 Protocol;       //协议头
  	 UINT32 Version;        //版本号 
  	 UINT32 Type;           //接口类型
  	 UINT32 Flags;          //协议标志位
	 UINT32 DataLength;   //???ó??3¤?è
	 UINT32 eType;
  	 UINT32 ExpandLength;   //附加项长度
  	 UINT32 Reserved;    //保留
} __attribute__((packed)) PROTOCOL_HEAD;


typedef struct tagPolicyProtocol
{
   PROTOCOL_HEAD Head;	
   BYTE * Data;           //数据内容   
   BYTE * eData;          //附加项内容
} __attribute__((packed)) POLICY_PROTOCOL;  

//策略请求数据包内容定义
typedef struct tagPolicyRequestPackage
{
    BYTE NodeSequence[20];     //节点序列号
    BYTE UserName[40];         //用户名
    UINT32 ExtendDataLength;   //扩展信息长度
    BYTE * ExtendData;         //扩展信息内容 
} __attribute__((packed)) POLICY_REQ_PACK; 

#define OPERATE_TYPE_EXEC     0x10
#define OPERATE_TYPE_RENAME   0x08
#define OPERATE_TYPE_DELETE   0x04
#define OPERATE_TYPE_WRITE    0x02
#define OPERATE_TYPE_READ     0x01

#define OBJECT_TYPE_SYSFILE   	0x10
#define OBJECT_TYPE_AUDITFILE 	0x08
#define OBJECT_TYPE_POLICYFILE 	0x04
#define OBJECT_TYPE_WORKFILE 	0x02
#define OBJECT_TYPE_CRYPT	0x01

typedef struct auditpolicytime
{
    BYTE Year[4];
    BYTE Month[2];
    BYTE Day[2];
    BYTE Hour[2];
    BYTE Min[2];
    BYTE Second[2];
    BYTE Week[2];
}APTIME;

// 审计策略数据结构
typedef struct tagAudit_Record
{
   UINT16 NodeID;  	     	//节点序列号
   UINT16 iType;       	 	//审计类型
   UINT32 Time;  	  	//审计事件发生时间
   BYTE SubName[20];           //主体名称
   MAC_LABEL SubLabel;	
   BYTE SubType;	
   BYTE ObjName[20];           //客体名称    
   MAC_LABEL ObjLabel;	
   BYTE ObjType;
   UINT16 Bret;       	 	  //审计类型
   BYTE Reserved[8];  		//保留字段
} __attribute__((packed)) AUDIT_RECORD;  

//扩展审计数据结构
typedef struct tagKAudit_Record            //wdh 20110601
{
	 UINT16 NodeID;         //节点编号
	 UINT16 iType;          //审计类型
	 UINT16 ExpandNo;       //扩展包序号
	 BYTE ExpandData[60];  //扩展数据
	 BYTE Reserved1[6];     //保留字段
	 BYTE Reserved[8];     //保留字段
}__attribute__((packed)) AUDIT_EXPANDRECORD;  

static int innum[5] ={0};
static int denum[5] ={0};
#endif
