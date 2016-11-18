/*************************************************
*       project:        973 trust demo, zhongan secure os 
*                       and trust standard verify
*	name:		extern_defno.h
*	write date:    	2011-08-04
*	auther:    	Hu jun
*       content:        this file describe the extern struct's format with
*       		attr struct array
*       changelog:       
*************************************************/
#ifndef _OS210_EXTERN_DEFNO_H
#define _OS210_EXTERN_DEFNO_H

// Trust Domain type definition
enum Domain_Type
{
	DTYPE_TSB,
	DTYPE_BASE,
	DTYPE_SYS,
	DTYPE_SEC,
	DTYPE_AUD,
	DTYPE_SRV,
	DTYPE_APP,
	DTYPE_USER,
	DTYPE_TEST
};

#define DTYPE_MASK 0x0F
#define DTYPE_TRUST_FLAG 0x10
#define DTYPE_CRYPT_FLAG 0x20
#define DTYPE_DELEGATE_FLAG 0x80
#define DTYPE_BACKUP_FLAG 0x40

#define DTYPE_SUB_TRUST_FLAG 0x10
// trust flag means that this sub has its special label convert function

#define DTYPE_SUB_SCRIPT_FLAG 0x20
// script flag means that either it provide a shell interface,
// or it exec another script program

#define DTYPE_SUB_REPLACE_FLAG 0x40
// replace flag means that we should use this sub's label replace the 
// current proc's label when the sub is executed 

#define DTYPE_SUB_APP_FLAG 0x80
// app flag means that we should only replace the current proc's label's
// name and app domain when the sub is executed.
// if either REPLACE_FLAG or APP_FLAG is not setted,we should only replace 
// the current proc's name when a sub is executed

#define AUDIT_TYPE_ACCESS   	  0x01
  enum audit_record_type
  {
	  NO_AUDIT_RECORD,
	  NORMAL_AUDIT_RECORD,
	  KEY_AUDIT_RECORD,
	  EXCEPT_AUDIT_RECORD	  
  };
enum rw_op_type
{
	KAUDIT_TYPE_SUB_CREATE=1,
	KAUDIT_TYPE_OBJ_CREATE,
	KAUDIT_TYPE_SUB_MARK,
	KAUDIT_TYPE_OBJ_MARK,
	KAUDIT_TYPE_EXEC,
	KAUDIT_TYPE_READ,
	KAUDIT_TYPE_WRITE,
	KAUDIT_TYPE_SUB_LOGIN,
	KAUDIT_TYPE_SUB_LOGOUT,
	KAUDIT_TYPE_SUB_EXIT,
	KAUDIT_TYPE_OBJ_DELETE,
	KAUDIT_TYPE_OBJ_RENAME,
//wdh 20110601
	KAUDIT_TYPE_SYS_START=0x40,
	KAUDIT_TYPE_IDENTIFY,
	KAUDIT_TYPE_SYS_HALT,

	KAUDIT_TYPE_POLICY_UPDATE=0x60,

	KAUDIT_TYPE_EXPAND_SINGLE=0x70,
	KAUDIT_TYPE_EXPAND_HEAD,
	KAUDIT_TYPE_EXPAND,
	KAUDIT_TYPE_EXPAND_TAIL,
	KAUDIT_TYPE_END
};
// Audit op type
#define AUDIT_OP_EXEC		0x01
#define AUDIT_OP_WRITE		0x02
#define AUDIT_OP_READ		0x03
#define AUDIT_OP_APPEND		0x04
#define AUDIT_OP_OPEN		0x05
#define AUDIT_OP_RENAME		0x06
#define AUDIT_OP_DELETE		0x07
#define AUDIT_OP_LOGIN		0x08
#define AUDIT_OP_LOGOUT		0x09
#define AUDIT_OP_POLICYADD	0x0A
#define AUDIT_OP_POLICYMOD	0x0B
#define AUDIT_OP_POLICYDEL	0x0C
#define AUDIT_OP_POLICYISSUE	0x0D
#define AUDIT_OP_KEYISSUE	0x0E
#define AUDIT_OP_FORK		0x21
#define AUDIT_OP_EXIT		0x22
#define AUDIT_OP_EXITGROUP	0x23
#define AUDIT_OP_SETUID		0x24
#define AUDIT_OP_CREATE		0x25
#define AUDIT_OP_CREATEFILE	0x26

//AUDIT OP desc

enum tpm_type
{
	PHYSICAL_TPM=1,
	VIRTUAL_TPM,
	REMOTE_TPM,
	USER_TPM
};

enum audit_probe_type
{
	AUDIT_PROBE_SYS_START=1,
	AUDIT_PROBE_TASK_INITMARK,
	AUDIT_PROBE_INODE_INITMARK,
	AUDIT_PROBE_FILE_INITMARK,
	AUDIT_PROBE_CREATE_INODE,
	AUDIT_PROBE_OPEN_FILE,
	AUDIT_PROBE_READ_FILE,
	AUDIT_PROBE_WRITE_FILE,
	AUDIT_PROBE_EXEC_FILE,
	AUDIT_PROBE_DELETE_FILE,
	AUDIT_PROBE_DELETE_DIR,
	AUDIT_PROBE_CREATE_DIR,
	AUDIT_PROBE_SET_INODE_ATTR,
	AUDIT_PROBE_GET_INODE_ATTR,
	AUDIT_PROBE_MKNOD,
	AUDIT_PROBE_RENAME,
	AUDIT_PROBE_NETWORK_ACCESS,
	AUDIT_PROBE_READ_INODE,
	AUDIT_PROBE_WRITE_INODE,
	AUDIT_PROBE_FORK,    //16
	AUDIT_PROBE_EXIT,
	AUDIT_PROBE_EXITGROUP,
	AUDIT_PROBE_LOGIN,
	AUDIT_PROBE_LOGOUT,
	AUDIT_PROBE_MSG_QUEUE_ASSOCIATE,
	AUDIT_PROBE_MSG_QUEUE_MSGCTL,
	AUDIT_PROBE_MSG_QUEUE_MSGSND,
	AUDIT_PROBE_MSG_QUEUE_MSGRCV,	
	AUDIT_PROBE_SHM_ASSOCIATE,
	AUDIT_PROBE_SHM_SHMCTL,
	AUDIT_PROBE_SHM_SHMAT,
	AUDIT_PROBE_SEM_ASSOCIATE,
	AUDIT_PROBE_SEM_SEMCTL,
	AUDIT_PROBE_SEM_SEMOP,
	AUDIT_PROBE_SOCKET_CREATE,
	AUDIT_PROBE_SOCKET_BIND,
	AUDIT_PROBE_SOCKET_IOCTL,
	AUDIT_PROBE_SOCKET_CONNECT,
	AUDIT_PROBE_SOCKET_LISTEN,
	AUDIT_PROBE_SOCKET_ACCEPT,
	AUDIT_PROBE_SOCKET_SENDMSG,
	AUDIT_PROBE_SOCKET_RECVMSG,
	AUDIT_PROBE_SOCKET_SETSOCKOPT,
	AUDIT_PROBE_SOCKET_GETSOCKOPT,
	AUDIT_PROBE_SETUID,
	AUDIT_PROBE_GET_INODE,
	AUDIT_PROBE_GET_FILE,
	AUDIT_PROBE_REPEAT_READ,
	AUDIT_PROBE_REPEAT_WRITE,
	AUDIT_PROBE_END,
};
#define AUDIT_TYPE_KEY   	0x01
#define AUDIT_TYPE_NORMAL   	0x00
#define AUDIT_TYPE_EXCEPT   	0x02

// the audit probe's mask
#define AUDIT_PROBE_ONTRUSTFAIL	0x10//trustfail audit
#define AUDIT_PROBE_ONPRIV  	0x08//priv succ audit
#define AUDIT_PROBE_ONMAC  	0x04//mac succ audit
#define AUDIT_PROBE_ONDAC  	0x02//dac succ audit
#define AUDIT_PROBE_ONFAIL  	0x01//fail audit
#define AUDIT_PROBE_ON	  	0x1F//all audit probe open
#define AUDIT_PROBE_ONSUCC  	0x0E//all audit
#define AUDIT_PROBE_OFF		0x00//audit probe off


// return value
#define VERIFY_DAC_SUCCESS	0x02 	//×ÔÖ÷·ÃÎÊ¿ØÖÆÍ¨¹ý	
#define VERIFY_DAC_FAILED	       0x03 	//×ÔÖ÷·ÃÎÊ¿ØÖÆÊ§°Ü(
					//(Ç¿ÖÆ·Ã¿ØÓò¼ì²éÊ§°ÜÒ²·µ»Ø¸ÃÖµ)
#define VERIFY_MAC_SUCCESS	0x04    //Ç¿ÖÆ·ÃÎÊ¿ØÖÆÍ¨¹ý
#define VERIFY_MAC_FAILED	      0x05 	//Ç¿ÖÆ·ÃÎÊ¿ØÖÆÊ§°Ü	
#define VERIFY_PRIV_SUCCESS	0x06 	//ÌØÈ¨¼ì²éÍ¨¹ý
#define VERIFY_PRIV_FAILED	0x07 	//ÌØÈ¨¼ì²éÊ§°Ü
#define VERIFY_SYS_SUCCESS 	0x08 	//ÏµÍ³¼ì²éÍ¨¹ý
#define VERIFY_SYS_FAILED 	0x09 	//ÏµÍ³¼ì²éÊ§°Ü
#define VERIFY_AUTH_SUCCESS 	0x0A 	//ÈÏÖ¤Í¨¹ý
#define VERIFY_AUTH_FAILED 	0x0B 	//ÈÏÖ¤Ê§°Ü
#define VERIFY_TRUST_SUCCEED 	0x0C 	//¿ÉÐÅÑéÖ¤Í¨¹ý
#define VERIFY_TRUST_FAILED 	0x0D 	//¿ÉÐÅÑéÖ¤Ê§°Ü
#define VERIFY_TRUST_NEEDCHECK 	0x00 	//¿ÉÐÅÑéÖ¤´ý¼ì²é
#define VERIFY_GENERAL_FAILED	0x01    //Ò»°ãÊ§°Ü      

#define ACC_DOM_ERR 1001
#define ACC_CONV_ERR 1002
#define ACC_MAC_ERR 1003
#define ACC_SUCCESS 0

// subject type
#define SUB_TYPE_SECADMIN	0x01       //°²È«¹ÜÀíÔ± 	
#define SUB_TYPE_SYSADMIN	0x02	   //ÏµÍ³¹ÜÀíÔ±
#define SUB_TYPE_AUDIT		0x03	   //°²È«Éó¼ÆÔ± 
#define SUB_TYPE_USER		0x04	   //²Ù×÷Ô±
#define SUB_TYPE_PROC		0x05	   //½ø³Ì
#define SUB_TYPE_DEV		0x06       //Éè±¸
#define SUB_TYPE_SYSTEM		0x80       //Éè±¸

// obj type
#define OBJ_TYPE_ENCRYPT	0x01	   //¼ÓÃÜ	
#define OBJ_TYPE_DEVICE		0x02	   //Éè±¸
#define OBJ_TYPE_SERVICE	0x04	   //ÏµÍ³·þÎñ
#define OBJ_TYPE_APPDATA	0x08	   //ÒµÎñÎÄ¼þ
#define OBJ_TYPE_POLICE		0x10	   //²ßÂÔÎÄ¼þ
#define OBJ_TYPE_AUDIT		0x20       //Éó¼ÆÎÄ¼þ
#define OBJ_TYPE_SYSTEM		0x40       //ÏµÍ³ÎÄ¼þ   
#define OBJ_TYPE_TRUST		0x80       //¿ÉÐÅÎÄ¼þ   

#define OPERATION_TYPE_EXEC	0x01  	   //Ö´ÐÐ 	
#define OPERATION_TYPE_WRITE	0x02       //Ð´
#define OPERATION_TYPE_READ	0x04       //¶Á
#define OPERATION_TYPE_APPEND	0x08       //Ìí¼ÓÐ´
#define OPERATION_TYPE_OPEN	0x10       //´ò¿ª
#define OPERATION_TYPE_RENAME	0x20       //¸ÄÃû
#define OPERATION_TYPE_DELETE	0x40       //É¾³ý

#define POLICY_TYPE_SUBLABEL "SUBL"
#define POLICY_TYPE_OBJLABEL "OBJL"
#define POLICY_TYPE_AUDIT "AUDI"
#define POLICY_TYPE_USER "AUUL"
#define POLICY_TYPE_DAC "DACL"

#define POLICY_TYPE_DIGESTLIST "DIGL"
#define POLICY_TYPE_FILEDIGESTLIST "FDIG"
#define POLICY_TYPE_PCRSET "PCRS"

#define POLICY_TYPE_SELECT "SELP"

#define MESSAGE_TYPE_IMAGEINFO "IMGI"    // image's information message
#define MESSAGE_TYPE_VMINFO "VM_I"    // image's information message
#define MESSAGE_TYPE_VMPOLICY "VM_P"    // vm's policy message
#define MESSAGE_TYPE_PCRSET_POLICY "PCRP"    // vm's policy message
#define MESSAGE_TYPE_FILE_POLICY "FILP"    // vm's policy message
#define MESSAGE_TYPE_USERTPM "USRT"    // user's tpm information 
#define MESSAGE_TYPE_PLATFORM_TPM "PLAT"    // user's tpm information 
#define MESSAGE_TYPE_VM_TPM "VM_T"    // user's tpm information 

#define MESSAGE_TYPE_TPM_PUB_KEY "PUBK"    // user's tpm information 
#define MESSAGE_TYPE_TPM_BLOB_KEY "BLBK"    // user's tpm information 

#define MESSAGE_TYPE_AUDIT "AUDR"    // audit data package
#define MESSAGE_TYPE_AUDIT "AUDR"    // audit data package
#define MESSAGE_TYPE_AUDIT "AUDR"    // audit data package
#define MESSAGE_TYPE_UPDATESTART "UPPS"    //update policy start
#define MESSAGE_TYPE_UPDATEEND "UPPR"    //update policy end
#define MESSAGE_TYPE_INSTALLSTART "INSS"    //Install/Modify/Uninstall start

#define TRUSTPORT   12580
#define MANAGER_TRUSTPORT   12580
#define PLATFORM_TRUSTPORT   12581
#define NETWORK_TRUSTPORT   12582
#define VM_TRUSTPORT   	12583
#define ENDPOINT_TRUSTPORT   12584


#endif
