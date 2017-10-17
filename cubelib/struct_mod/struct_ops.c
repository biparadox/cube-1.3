#include "../include/errno.h"
#include "../include/data_type.h"
#include "../include/alloc.h"
#include "../include/memfunc.h"
#include "../include/struct_deal.h"
#include "struct_ops.h"
#include "struct_attr.h"


//const int deftag=0x00FFF000;

UINT16
Decode_UINT16(BYTE * in)
{
	UINT16 temp = 0;
	temp = (in[1] & 0xFF);
	temp |= (in[0] << 8);
	return temp;
}

void
UINT32ToArray(UINT32 i, BYTE * out)
{
	out[0] = (BYTE) ((i >> 24) & 0xFF);
	out[1] = (BYTE) ((i >> 16) & 0xFF);
	out[2] = (BYTE) ((i >> 8) & 0xFF);
	out[3] = (BYTE) i & 0xFF;
}

void
UINT64ToArray(UINT64 i, BYTE *out)
{
	out[0] = (BYTE) ((i >> 56) & 0xFF);
	out[1] = (BYTE) ((i >> 48) & 0xFF);
	out[2] = (BYTE) ((i >> 40) & 0xFF);
	out[3] = (BYTE) ((i >> 32) & 0xFF);
	out[4] = (BYTE) ((i >> 24) & 0xFF);
	out[5] = (BYTE) ((i >> 16) & 0xFF);
	out[6] = (BYTE) ((i >> 8) & 0xFF);
	out[7] = (BYTE) i & 0xFF;
}

void
UINT16ToArray(UINT16 i, BYTE * out)
{
	out[0] = ((i >> 8) & 0xFF);
	out[1] = i & 0xFF;
}

UINT64
Decode_UINT64(BYTE *y)
{
	UINT64 x = 0;

	x = y[0];
	x = ((x << 8) | (y[1] & 0xFF));
	x = ((x << 8) | (y[2] & 0xFF));
	x = ((x << 8) | (y[3] & 0xFF));
	x = ((x << 8) | (y[4] & 0xFF));
	x = ((x << 8) | (y[5] & 0xFF));
	x = ((x << 8) | (y[6] & 0xFF));
	x = ((x << 8) | (y[7] & 0xFF));

	return x;
}

UINT32
Decode_UINT32(BYTE * y)
{
	UINT32 x = 0;

	x = y[0];
	x = ((x << 8) | (y[1] & 0xFF));
	x = ((x << 8) | (y[2] & 0xFF));
	x = ((x << 8) | (y[3] & 0xFF));
}

char *dup_str(char * src, int size)
{
	char * dest;
	int len;
	int ret;
	dest=NULL;
	if(src==NULL)
		return 0;
	if(size == 0)
	{
		len=Strlen(src)+1;
	}
	else
	{
		len=Strnlen(src,size);
		if(len!=size)
			len++;
	}
	
	dest=Talloc(len);	
	if(dest==NULL)
		return dest;
	Memcpy(dest,src,len);
	return dest;			
}

int estring_get_length (void * value,void * attr)
{

	struct elem_template * elem_attr=attr;
	struct struct_elem_attr * elem_desc = elem_attr->elem_desc;
	int retval;
	if(value==NULL)
		return 0;
	retval = Strlen(value);
	if (retval<0)
		retval = -EINVAL;
	retval++;
	return retval;
}

int _digest_to_uuid(BYTE * digest, char * uuid)
{
	BYTE char_value;
	int i,j,k;
	k=0;
	for(i=0;i<DIGEST_SIZE;i++)
	{
		int tempdata;
		char_value=digest[i];

		for(j=0;j<2;j++)
		{
			tempdata=char_value>>4;
			if(tempdata>9)
				*(uuid+k)=tempdata-10+'a';
			else
				*(uuid+k)=tempdata+'0';
			k++;
			if(j!=1)
				char_value<<=4;
				
		}
	}
	return DIGEST_SIZE*2;
}

int _uuid_to_digest(char * uuid,BYTE * digest)
{
	int i;
	BYTE char_value;
	for(i=0;i<DIGEST_SIZE*2;i++)
	{
		if((uuid[i]>='0')&&(uuid[i]<='9'))
			char_value=uuid[i]-'0';
		else if((uuid[i]>='a')&&(uuid[i]<='f'))
			char_value=uuid[i]-'a'+10;
		else if((uuid[i]>='A')&&(uuid[i]<='F'))
			char_value=uuid[i]-'A'+10;
		else
			return -EINVAL;
		if(i%2==0)
			digest[i/2]=char_value<<4;
		else
			digest[i/2]+=char_value;
	}
	return 0;
}

int uuid_get_text_value(void * addr, void * data,void * elem_template)
{

	if(Isstrinuuid(addr))
	// Now it is a name
	{
		Memset(data,0,DIGEST_SIZE);
		Strncpy(data,addr,DIGEST_SIZE/4*3);	
		return Strlen(data);
	}
	
	return _digest_to_uuid(addr,data);
}

int uuid_set_text_value(void * addr, char * text,void * elem_template)
{
	if(Strnlen(text,DIGEST_SIZE*2)<DIGEST_SIZE/4*3)
	{
		// Now it is a name
		Memset(addr,0,DIGEST_SIZE);
		Strncpy(addr,text,DIGEST_SIZE/4*3);	
		return DIGEST_SIZE;
		
	}
	return _uuid_to_digest(text,addr);
}

int namelist_get_bin_value(void * addr, void * data,void * elem_template)
{
	NAME2VALUE * namelist=addr;

	int addroffset=0;
	int offset=0;
	struct elem_template * curr_elem=elem_template;
	int textlen=0;
	textlen=Strlen(*(char **)namelist);
	if(textlen>DIGEST_SIZE)
		return -EINVAL;
	Memcpy(data,*(char **)namelist,textlen+1);
	
	offset+=textlen+1;
	Memcpy(data+offset,&namelist->value,sizeof(int));
	offset+=sizeof(int);
	return offset;
}

int namelist_set_bin_value(void * addr, void * data,void * elem_template)
{
	int retval;
	NAME2VALUE * namelist=addr;
	struct elem_template * curr_elem=elem_template;
	int offset=0;
	int addroffset=0;
	int textlen=0;
	textlen=Strlen((char *)data);
	if(textlen>DIGEST_SIZE)
		return -EINVAL;
	namelist->name=Dalloc0(textlen+1,namelist);
	if(namelist->name==NULL)
		return -ENOMEM;
	Memcpy(*(char **)namelist,data+offset,textlen+1);
	offset+=textlen+1;
	Memcpy(&namelist->value,data+offset,sizeof(int));
	offset+=sizeof(int);
	return offset;
}

int namelist_clone(void * addr, void * clone,void * elem_template)
{
	int retval;
	NAME2VALUE * namelist=addr;
	

	NAME2VALUE * clonelist=clone;
	struct elem_template * curr_elem=elem_template;
	int textlen=0;
	textlen=Strlen(*(char **)namelist);
	if(textlen>DIGEST_SIZE)
		return -EINVAL;
	clonelist->name=Dalloc0(textlen+1,clone);
	if(clonelist->name==NULL)
		return -ENOMEM;
	Memcpy(*(char **)clone,*(char **)namelist,textlen+1);
	clonelist->value=namelist->value;
	return 0;
}

int namelist_get_text_value(void * addr, void * data,void * elem_template)
{
	NAME2VALUE * namelist=addr;
	int addroffset=0;
	int offset=0;
	char * text=data;
	char * name;
	UINT32  value;
	struct elem_template * curr_elem=elem_template;
	int textlen=0;

	textlen=Strlen(namelist->name);
	if(textlen>DIGEST_SIZE)
		return -EINVAL;
	Memcpy(text+offset,namelist->name,textlen);
	offset+=textlen;
	value=namelist->value;

	curr_elem->index++;
	if(value<curr_elem->index)
		return -EINVAL;
	if(value>curr_elem->index)
	{
		text[offset++]='=';
		textlen=Itoa(value,data+offset);
		if(textlen<0)
			return textlen;
		offset+=textlen;
		curr_elem->index=value;
	}
	text[offset++]=',';
	text[offset++]=0;
	return offset;
}

int namelist_set_text_value(void * addr, char * text,void * elem_template)
{
	int i,j,retval;
	NAME2VALUE * namelist=addr;
	struct elem_template * curr_elem=elem_template;
	int offset=0;
	int addroffset=0;
	int textlen=0;
	int namelen=0;
	char * name;
	UINT32  value;

	char buf[128];

	textlen=Getfiledfromstr(buf,text+offset,',',128);
	if(textlen>DIGEST_SIZE*2)
		return -EINVAL;
	namelen=Getfiledfromstr(buf+DIGEST_SIZE*2,buf,'=',textlen);
	curr_elem->index++;
	if(namelen==textlen)
	{
		value=curr_elem->index;
	}
	else
	{
		value=Atoi(buf+namelen+1,textlen-namelen);
		if(value<curr_elem->index)
			return -EINVAL;
		curr_elem->index=value;
	}
	namelist->name=Dalloc0(namelen+1,namelist);
	if(namelist->name==NULL)
		return -ENOMEM;
	Memcpy(namelist->name,text+offset,namelen);
	*(namelist->name+namelen)=0;
	offset+=textlen+1;
	namelist->value=value;
	return offset;
}

int define_get_text_value(void * addr,char * text,void * elem_template){
	int ret;
	struct elem_template * curr_elem=elem_template;
	int def_value;

	def_value=curr_elem->tempsize;
	if(def_value<=0)
		return def_value;
	ret=bin_to_radix64(text,def_value,addr);
	return ret;	
//	ret=bin_to_radix64(addr,curr_elem->size,elem);

//	Memcpy(text,blob,def_value);
//	return def_value;
}

int define_set_text_value(void * addr,char * text,void * elem_template){
	int ret;
	struct elem_template * curr_elem=elem_template;
	int def_value;
	int str_len;

	def_value=curr_elem->tempsize;
	if(def_value<=0)
		return def_value;
	str_len=bin_to_radix64_len(def_value);

	ret=radix64_to_bin(addr,str_len,text);
	return ret;	
}

static inline int _isdigit(char c)
{
	if((c>='0') && (c<='9'))
		return 1;
	return 0;
}

static inline int _get_char_value(char c)
{
	if(_isdigit(c))
		return c-'0';
	if((c>='a') && (c<='f'))
		return c-'a'+9;
	if((c>='A') && (c<='F'))
		return c-'a'+9;
	return -EINVAL;
}

int get_string_value(void * addr,void * elem_attr)
{
	struct elem_template * curr_elem=elem_attr;
	char * string=addr;
	int ret=0;
	int i;
	int base=10;
	int temp_value;
	int str_len;
	if(curr_elem->elem_desc->type == CUBE_TYPE_STRING)
	{
		str_len=Strnlen(string,curr_elem->size);
	}
	else
		str_len=Strnlen(string,16);

	// process the head
	for(i=0;i<str_len;i++)
	{
		if(string[i]==0)
			break;
		if(string[i]==' ')
		{
			i++;
			continue;
		}
		// change the base
		if(string[i]=='0')
		{
			switch(string[i+1])
			{
				case 0:
					return 0;
				case 'b':
				case 'B':
					i+=2;
					base=2;
					break;
				case 'x':
				case 'X':
					i+=2;
					base=16;
					break;
				default:
					i++;
					base=8;
					break;

			}
			break;
		}
		
	}
	for(;i<str_len;i++)
	{
		if(string[i]==0)
			break;
		temp_value=_get_char_value(string[i]);
		if((temp_value <0)||(temp_value>=base))
			return -EINVAL;
		ret=ret*base+temp_value;		
	}
	return ret;
}
	
int get_int_value(void * addr,void * elem_attr)
{
	return *(int *)addr; 
}

int int_get_text_value(void * elem,char * text,void * elem_attr)
{
	struct elem_template * curr_elem=elem_attr;
	int i;
	long long value=0;
	int len;
	char buffer[DIGEST_SIZE];
	char *pch=text;

	Memcpy(&value,elem,curr_elem->size);
	if(curr_elem->size==8)
	{
		if(value<0)
		{
			*pch++='-';
			value=-value;
		}
	}
	else
	{
		long long comp_value=((long long)1)<<(curr_elem->size*8-1);
		if(value>comp_value)
		{
			comp_value<<=1;
			value=comp_value-value;
			*pch++='-';
		}
	}	

	i=1;
	len=2;
	while(value/i)
	{
		i*=10;
		len++;
	}
	if(value==0)
		i=10;
	while(i/=10)
	{
		*pch++=value/i+'0';
		value%=i;
	}	
	*pch='\0';
	return len;
}
int int_set_text_value(void * addr,char * text,void * elem_attr)
{
	struct elem_template * curr_elem=elem_attr;
	char * string=text;
	int ret=0;
	int i;
	int base=10;
	int temp_value;

	int str_len;

	str_len=Strnlen(string,DIGEST_SIZE);

	// process the head
	for(i=0;i<str_len;i++)
	{
		if(string[i]==0)
			break;
		if(string[i]==' ')
		{
			i++;
			continue;
		}
		// change the base
		if(string[i]=='0')
		{
			switch(string[i+1])
			{
				case 0:
					ret=0;
					Memcpy(addr,&ret,curr_elem->size);
					return str_len+1;
				case 'b':
				case 'B':
					i+=2;
					base=2;
					break;
				case 'x':
				case 'X':
					i+=2;
					base=16;
					break;
				default:
					i++;
					base=8;
					break;

			}
		}
		break;
	}
	for(;i<str_len;i++)
	{
		if(string[i]==0)
			break;
		temp_value=_get_char_value(string[i]);
		if((temp_value <0)||(temp_value>=base))
			return -EINVAL;
		ret=ret*base+temp_value;		
	}
	Memcpy(addr,&ret,curr_elem->size);
	return str_len+1;
}

int tpm64_get_bin_value(void * elem,void * addr,void * elem_attr)
{
	UINT64ToArray(*(int *)elem,addr);
	return sizeof(UINT64);
}

int tpm64_set_bin_value(void * elem,void * addr,void * elem_attr)
{
	*(UINT64 *)elem=Decode_UINT64(addr);
	return sizeof(UINT64);
}

int tpm32_get_bin_value(void * elem,void * addr,void * elem_attr)
{
	UINT32ToArray(*(int *)elem,addr);
	return sizeof(UINT32);
}

int tpm32_set_bin_value(void * elem,void * addr,void * elem_attr)
{
	*(UINT32 *)elem=Decode_UINT32(addr);
	return sizeof(UINT32);
}

int tpm16_get_bin_value(void * elem,void * addr,void * elem_attr)
{
	UINT16ToArray(*(int *)elem,addr);
	return sizeof(UINT16);
}

int tpm16_set_bin_value(void * elem,void * addr,void * elem_attr)
{
	*(UINT16 *)elem=Decode_UINT16(addr);
	return sizeof(UINT16);
}
int bindata_get_text_value(void *elem,void * addr,void * elem_attr)
{
	int ret;
	struct elem_template * curr_elem=elem_attr;
	if(curr_elem->size==0)
		return 0;
	ret=bin_to_radix64(addr,curr_elem->size,elem);
	return ret;
}

int bindata_set_text_value(void *elem,void * addr,void * elem_attr)
{
	int ret;
	int len;
	struct elem_template * curr_elem=elem_attr;
	if(curr_elem->size==0)
		return 0;
	len=Strlen((char *)addr);
	if(len==0)
		return 0;
	ret=radix_to_bin_len(len);
	if(ret!=curr_elem->size)
		return -EINVAL;
	ret=radix64_to_bin(elem,len,addr);
	return ret;
}

int hexdata_get_text_value(void *elem,void * addr,void * elem_attr)
{
	int ret;
	struct elem_template * curr_elem=elem_attr;
	if(curr_elem->size==0)
		return 0;
	ret=bin_2_hex(elem,curr_elem->size,addr);
	return ret;
}

int hexdata_set_text_value(void *elem,void * addr,void * elem_attr)
{
	int ret;
	int len;
	struct elem_template * curr_elem=elem_attr;
	if(curr_elem->size==0)
		return 0;
	ret=hex_2_bin(addr,curr_elem->size,elem);
	return ret;
}


ELEM_OPS string_convert_ops =
{
	.get_int_value=get_string_value,
};

ELEM_OPS bindata_convert_ops =
{
	.get_text_value=bindata_get_text_value,
	.set_text_value=bindata_set_text_value
};
ELEM_OPS hexdata_convert_ops =
{
	.get_text_value=hexdata_get_text_value,
	.set_text_value=hexdata_set_text_value
};

ELEM_OPS uuid_convert_ops =
{
	.get_text_value = uuid_get_text_value,
	.set_text_value = uuid_set_text_value,
};

ELEM_OPS uuidarray_convert_ops =
{
	.get_text_value = uuid_get_text_value,
	.set_text_value = uuid_set_text_value,
};

ELEM_OPS defuuidarray_convert_ops =
{
//	.get_text_value = defuuidarray_get_text_value,
//	.set_text_value = defuuidarray_set_text_value,
	.get_text_value = uuid_get_text_value,
	.set_text_value = uuid_set_text_value,
};
ELEM_OPS estring_convert_ops =
{
	.elem_get_length = estring_get_length,
};
ELEM_OPS define_convert_ops =
{
	.get_text_value = define_get_text_value,
	.set_text_value = define_set_text_value,
};

ELEM_OPS int_convert_ops =
{
	.get_int_value=get_int_value,
	.get_text_value = int_get_text_value,
	.set_text_value = int_set_text_value,
};
ELEM_OPS defnamelist_convert_ops =
{
	.clone_elem= namelist_clone,
	.get_bin_value= namelist_get_bin_value,
	.set_bin_value= namelist_set_bin_value,
	.get_text_value= namelist_get_text_value,
	.set_text_value= namelist_set_text_value,
};

ELEM_OPS tpm_uint64_convert_ops =
{
	.get_bin_value= tpm64_get_bin_value,
	.set_bin_value= tpm64_set_bin_value,
	.get_text_value= int_get_text_value,
	.set_text_value= int_set_text_value,
};

ELEM_OPS tpm_uint32_convert_ops =
{
	.get_bin_value= tpm32_get_bin_value,
	.set_bin_value= tpm32_set_bin_value,
	.get_text_value= int_get_text_value,
	.set_text_value= int_set_text_value,
};
ELEM_OPS tpm_uint16_convert_ops =
{
	.get_bin_value= tpm16_get_bin_value,
	.set_bin_value= tpm16_set_bin_value,
	.get_text_value= int_get_text_value,
	.set_text_value= int_set_text_value,
};
