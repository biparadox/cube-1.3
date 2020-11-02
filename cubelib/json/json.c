#include "../include/list.h"

#include "../include/data_type.h"
#include "../include/errno.h"
#include "../include/memfunc.h"
#include "../include/json.h"
#include "../include/alloc.h"
#include "../include/attrlist.h"
#include "json_internal.h"
#define DIGEST_SIZE 32


static inline int IsValueEnd(char c)
{
    if((c==' ')||(c==',')||(c=='\0')||(c=='}')||(c==']')||(c=='\t')||(c=='\r')||(c=='\n'))
    {
        return 1;
    }
    return 0;
}
static inline int IsSplitChar(char c)
{
    if((c==',')||(c==' ')||(c=='\r')||(c=='\n')||(c=='\t'))
    {
        return 1;
    }
    return 0;
}

static inline int IsIgnoreChar(char c)
{
    if((c==' ')||(c=='\r')||(c=='\n')||(c=='\t'))
    {
        return 1;
    }
    return 0;
}

void * json_get_first_child(void * father)
{
    JSON_NODE * father_node = (JSON_NODE *)father;
    father_node->curr_child =(Record_List *) (father_node->childlist.list.next);
    if(father_node->curr_child == &(father_node->childlist))
        return NULL;
    return father_node->curr_child->record;
}

void * json_get_next_child(void * father)
{
    JSON_NODE * father_node = (JSON_NODE *)father;
    if(father_node->curr_child == &(father_node->childlist))
        return NULL;
    father_node->curr_child = (Record_List *)father_node->curr_child->list.next;
    return father_node->curr_child->record;
}

int json_remove_node(void * node)
{
    JSON_NODE * curr_node=(JSON_NODE *)node;
    JSON_NODE * father_node=curr_node->father;
    Record_List * temp_node;

    if(father_node==NULL)
	return -EINVAL;	
    if(father_node->curr_child->record==node)
    {
	father_node->curr_child=(Record_List *)(father_node->curr_child->list.next);
    }	
    
    temp_node=(Record_List *)(father_node->childlist.list.next);
    while(temp_node->record!=node)
    {
	if(temp_node==&(father_node->childlist))
		return -EINVAL;
	temp_node=(Record_List *)(temp_node->list.next);
    }		
    temp_node->list.next->prev=temp_node->list.prev;
    temp_node->list.prev->next=temp_node->list.next;
    temp_node->list.prev=&(temp_node->list);
    temp_node->list.next=&(temp_node->list);
	
    father_node->elem_no--;
    return 0;
}

void * json_get_father(void * child)
{
    if(child==NULL)
	return NULL;
    JSON_NODE * child_node = (JSON_NODE *)child;
    return child_node->father;
}

int json_get_type(void * node)
{
    if(node==NULL)
		return -EINVAL;
    JSON_NODE * json_node = (JSON_NODE *)node;
    return json_node->elem_type &JSON_ELEM_MASK;
}

int json_is_value(void * node)
{
    if(node==NULL)
		return -EINVAL;
    JSON_NODE * json_node = (JSON_NODE *)node;
    return json_node->elem_type & JSON_ELEM_VALUE;
}

char * json_get_valuestr(void * node)
{
    if(node==NULL)
	return NULL;
    JSON_NODE * json_node = (JSON_NODE *)node;
    return json_node->value_str;
}

char * json_set_valuestr(void * node,char * str)
{
    char * temp_str;
    if(node==NULL)
		return NULL;
    JSON_NODE * json_node = (JSON_NODE *)node;
    temp_str= json_node->value_str;
    json_node->value_str=str;
    return temp_str;
}

int json_get_elemno(void * node)
{
    int no;
    struct List_head * root;
    struct List_head * child;
    if(node==NULL)
		return -EINVAL;
	
    JSON_NODE * json_node = (JSON_NODE *)node;
    return json_node->elem_no;
}

int json_comp_elemno(void * node)
{
    JSON_NODE * json_node = (JSON_NODE *)node;
    int type =json_get_type(json_node);
    if((type==JSON_ELEM_MAP) ||
    	(type==JSON_ELEM_ARRAY))
    {
	json_node->elem_no = get_record_List_no(&json_node->childlist);
	return json_node->elem_no;
    }
    return 0;	
}

Record_List * get_new_Record_List(void * record)
{
    Record_List * newrecord = Calloc(sizeof(Record_List));
    if(newrecord == NULL)
        return NULL;
    INIT_LIST_HEAD (&(newrecord->list));
    newrecord->record=record;
    return newrecord;
}

static inline int json_get_numvalue(void * value_addr,char * json_str)
{
    int i=0;
    int value=0;
    int isnegative=0;

     if(json_str[0]!='.')
    {
	if(json_str[0]=='-')
	{
		isnegative=1;
		i++;
	}
        if((json_str[i]<'0')||(json_str[i]>'9'))
            return -EINVAL;
	value=json_str[i++]-'0';
    }
    for(;i<DIGEST_SIZE;i++)
    {
        if(json_str[i]==0)
            return -EINVAL;
        if(IsValueEnd(json_str[i]))
            break;
        if((json_str[i]<'0')||(json_str[i]>'9'))
            return -EINVAL;
	value=value*10+(json_str[i]-'0');
    }
    if(i==DIGEST_SIZE)
        return -EINVAL;

    if(isnegative)
    {
    	*((int *)value_addr)=-value;
    }
    else
    {
    	*((int *)value_addr)=value;
    }
    return i;
}

void * json_find_elem(char * name,void * root)
{
	JSON_NODE * root_node = (JSON_NODE * )root;
	JSON_NODE * this_node ;

	if(json_get_type(root_node)!=JSON_ELEM_MAP)
		return NULL;
        this_node = (JSON_NODE *)json_get_first_child(root);
	
	while(this_node != NULL)
	{
		if(Strncmp(name,this_node->name,DIGEST_SIZE)==0)
			break;
		this_node=(JSON_NODE *)json_get_next_child(root);
	}
	return (void *)this_node;
}

static inline int json_get_boolvalue(void * value_addr,char * json_str)
{
   int i;
   const char * truestring[3]={"true","True","TRUE"};
   const char * falsestring[3]={"false","False","FALSE"};

   switch(json_str[0])
   {
	case 't':
		if(Memcmp(json_str,"true",4)==0)
		{
			*(int *)value_addr=1;
			i=4;
			break;
		}
		else
			return -EINVAL;
	case 'T':
		if(Memcmp(json_str,"True",4)==0)
		{
			*(int *)value_addr=1;
			i=4;
			break;
		}
		else if(Memcmp(json_str,"TRUE",4)==0)
		{
			*(int *)value_addr=1;
			i=4;
			break;
		}
		else
			return -EINVAL;
	case 'f':
		if(Memcmp(json_str,"false",5)==0)
		{
			*(int *)value_addr=1;
			i=5;
			break;
		}
		else
			return -EINVAL;
	case 'F':
		if(Memcmp(json_str,"False",5)==0)
		{
			*(int *)value_addr=1;
			i=5;
			break;
		}
		else if(Memcmp(json_str,"FALSE",5)==0)
		{
			*(int *)value_addr=1;
			i=5;
			break;
		}
		else
			return -EINVAL;

	default:
		return -EINVAL;
   }
   return i;
}

static inline int json_get_nullvalue(char * valuestr,char * json_str)
{
    int i;
    if((json_str[0]!='n')||(json_str[0]!='N'))
        return -EINVAL;
   for(i=0;i<6;i++)
   {
       if(json_str[i]==0)
           return -EINVAL;
       if(IsValueEnd(json_str[i]))
           break;
       valuestr[i]=json_str[i];
   }
   if(i==0)
        return -EINVAL;
   if(i==6)
        return -EINVAL;
   valuestr[i]=0;
   return i;

}

static inline int json_get_strvalue(char * valuestr,char * json_str)
{
    int i;
    int offset=0;
   if(json_str[0]!='"')
	return -EINVAL;
    for(i=1;i<1024;i++)
    {
        if(json_str[i]=='\"')
            break;
        if(json_str[i]==0)
            return -EINVAL;
	if(json_str[i]=='\\')
	    valuestr[offset]=json_str[++i];
	else
            valuestr[offset]=json_str[i];
	offset++;
    }
    if(i==1024)
        return -EINVAL;
    valuestr[offset]=0;
    return i+1;
}

void * _new_json_node(void * father)
{
    JSON_NODE * newnode;
    JSON_NODE * father_node=(JSON_NODE *)father;
    newnode=Calloc(sizeof(JSON_NODE));
    if(newnode==NULL)
        return NULL;
    Memset(newnode,0,sizeof(JSON_NODE));
    INIT_LIST_HEAD(&(newnode->childlist.list));
    newnode->father=father;
    Record_List * newrecord = get_new_Record_List(newnode);
    if(newrecord == NULL)
        return NULL;
    if(father_node!=NULL)
    {
        List_add_tail((struct List_head *)newrecord,&(father_node->childlist.list));
        father_node->curr_child=newrecord;
    }
    return newnode;
}

int json_set_type(void * node,int datatype,int isvalue)
{
    int ret=0;
    JSON_NODE * json_node=(JSON_NODE *)node;
    if(node==NULL)
	return -EINVAL;
    if((datatype<JSON_ELEM_NULL) || (datatype>JSON_ELEM_ARRAY))
	return -EINVAL;
    if(isvalue)
    {
	json_node->elem_type |=JSON_ELEM_VALUE;	
	ret=JSON_ELEM_VALUE;
    } 
    if(datatype==JSON_ELEM_NULL)
	return ret;
  
    json_node->elem_type =ret | datatype;
    return json_node->elem_type;	
}

void * json_add_child(void * node,void * child)
{
    JSON_NODE * curr_node = (JSON_NODE *)node;
	
    Record_List * newrecord = get_new_Record_List(child);
    if(newrecord == NULL)
        return NULL;
    List_add((struct List_head *)newrecord,&(curr_node->childlist.list));
    curr_node->curr_child=newrecord;
    return newrecord;
	
}

int json_solve_str(void ** root, char *str)
{
    JSON_NODE * root_node;
    JSON_NODE * father_node;
    JSON_NODE * curr_node;
    JSON_NODE * child_node;
    int value_type;
    char value_buffer[512];

    char * tempstr;
    int i;
    int offset=0;
    int state=0;
    int ret;

    // state has three 

    // give the root node value

    root_node=_new_json_node(NULL);
    if(root_node==NULL)
        return -ENOMEM;
    father_node=NULL;
    curr_node=root_node;

    while(str[offset]!='\0')
    {
        if(IsSplitChar(str[offset]))
	{
              offset++;
              continue;
        }
        switch(json_get_type(curr_node))
        {
            case JSON_ELEM_NULL:
                while(str[offset]!=0)
                {
                    if(!IsSplitChar(str[offset]))
                        break;
                    offset++;
                }
		switch(str[offset])
		{
			case '{':
				json_set_type(curr_node,JSON_ELEM_MAP,1);
				break;
				
			case '[':
				json_set_type(curr_node,JSON_ELEM_ARRAY,1);
				break;

			case '"':
			default:
				return -EINVAL;
		}

                // get an object node,then switch to the SOLVE_OBJECT
		offset++;
                break;
           case JSON_ELEM_MAP:
                // if this map is empty,then finish this MAP
                if(str[offset]=='}')
                {
                    offset++;
		    curr_node=json_get_father(curr_node);
		    if(curr_node!=NULL)
			curr_node->elem_no++;
		    break;
                }
                // if this value is an empty value
		if(str[offset]==',')
		{
                    offset++;
		    break;
		}
                // if we should to find another elem
		if(str[offset]!='\"')
			return -EINVAL;
		child_node=_new_json_node(curr_node);
                ret=json_get_strvalue(value_buffer,str+offset);
                if(ret<0)
                    return ret;
                if(ret>=DIGEST_SIZE)
                    return ret;
                offset+=ret;
		{
		    int len=Strlen(value_buffer);
		    if(len<=DIGEST_SIZE)
               	        Memcpy(child_node->name,value_buffer,len+1);
		    else
               	        Memcpy(child_node->name,value_buffer,DIGEST_SIZE);
		}
		while(str[offset]!='\0')
		{
        	    if(!IsIgnoreChar(str[offset]))
		    {		
              	        break;
        	    }
             	    offset++;
		}
		if(str[offset]!=':')
		    return -EINVAL;
		offset++;
		while(str[offset]!='\0')
		{
        	    if(!IsIgnoreChar(str[offset]))
		    {		
              	        break;
        	    }
             	    offset++;
		}

		// if this value is a map
		if(str[offset]=='{')
		{
			json_set_type(child_node,JSON_ELEM_MAP,0);
			father_node=curr_node;
			curr_node=child_node;
			offset++;	
			break;	
		}
		// if this value is an array
		if(str[offset]=='[')
		{
			json_set_type(child_node,JSON_ELEM_ARRAY,0);
			father_node=curr_node;
			curr_node=child_node;
			offset++;	
			break;	
		}
		// if this value is a string
			
		else if(str[offset]=='"')
	        {
                	ret=json_get_strvalue(value_buffer,str+offset);
                	if(ret<0)
                    		return ret;
                	if(ret>DIGEST_SIZE*8+2)
                    		return -EINVAL;
			json_set_type(child_node,JSON_ELEM_STRING,0);
			child_node->value_str=Dalloc(ret,child_node);
			if(child_node->value_str==NULL)
				return -ENOMEM;
			Memcpy(child_node->value_str,value_buffer,ret);
		}
		// if this value is a num
		else if((str[offset]=='-') ||
				((str[offset] >='0') && (str[offset]<='9')))
		{
                	ret=json_get_numvalue(value_buffer,str+offset);
                	if(ret<0)
                    		return ret;
                	if(ret>=DIGEST_SIZE)
                    		return ret;
			json_set_type(child_node,JSON_ELEM_NUM,0);
			Memcpy(&(child_node->value),value_buffer,sizeof(int));
		}
		// if this value is a bool
		else 
		{
                	ret=json_get_boolvalue(value_buffer,str+offset);
                	if(ret<0)
                    		return ret;
                	if(ret>=DIGEST_SIZE)
                    		return ret;
			json_set_type(child_node,JSON_ELEM_BOOL,0);
			Memcpy(&(child_node->value),value_buffer,sizeof(int));

		}
		offset+=ret;
		curr_node->elem_no++;
		break;
           case JSON_ELEM_ARRAY:

                // if this array meets the end(we should except the statement
                // that it is the procceed of the upper program
                if(str[offset]==']')
                {
                    offset++;
		    curr_node=json_get_father(curr_node);
		    if(curr_node!=NULL)
			curr_node->elem_no++;
		    break;
                }
                // if this value is an empty value
		if(str[offset]==',')
		{
                    offset++;
		    break;
		}
		// if this value is a map
		child_node=_new_json_node(curr_node);
		if(str[offset]=='{')
		{
			json_set_type(child_node,JSON_ELEM_MAP,1);
			father_node=curr_node;
			curr_node=child_node;
			offset++;	
			break;	
		}
		// if this value is an array
		if(str[offset]=='[')
		{
			json_set_type(child_node,JSON_ELEM_ARRAY,1);
			father_node=curr_node;
			curr_node=child_node;
			offset++;	
			break;	
		// if this value is a string
		}	
		else if(str[offset]=='"')
	        {
                	ret=json_get_strvalue(value_buffer,str+offset);
                	if(ret<0)
                    		return ret;
                	if(ret>DIGEST_SIZE*16+2)
                    		return -EINVAL;
			json_set_type(child_node,JSON_ELEM_STRING,1);
			child_node->value_str=Dalloc(ret,child_node);
			if(child_node->value_str==NULL)
				return -ENOMEM;
			Memcpy(child_node->value_str,value_buffer,ret);
		}
		// if this value is a num
		else if((str[offset] >='0') && (str[offset]<='9'))
		{
                	ret=json_get_numvalue(value_buffer,str+offset);
                	if(ret<0)
                    		return ret;
                	if(ret>=DIGEST_SIZE)
                    		return ret;
			json_set_type(child_node,JSON_ELEM_NUM,1);
			Memcpy(&(child_node->value),value_buffer,sizeof(int));
		}
		// if this value is a bool
		else 
		{
                	ret=json_get_boolvalue(value_buffer,str+offset);
                	if(ret<0)
                    		return ret;
                	if(ret>=DIGEST_SIZE)
                    		return ret;
			json_set_type(child_node,JSON_ELEM_BOOL,1);
			Memcpy(&(child_node->value),value_buffer,sizeof(int));

		}
		offset+=ret;
		curr_node->elem_no++;
		break;
            default:
                return -EINVAL;
        }
	if(curr_node==NULL)
		break;
    }
    *root=root_node;
    return offset;
}

int json_print_str(void * root,char * str)
{
    int offset=0;
    JSON_NODE * root_node=(JSON_NODE *)root;
    JSON_NODE * father_node;
    JSON_NODE * curr_node=root_node;
    JSON_NODE * child_node;
    int len;

    json_node_set_no(root_node,0);
    do
    {
	switch(json_get_type(curr_node))
	{
		case JSON_ELEM_MAP:
			if(curr_node->elem_no==0)
			{
				str[offset++]='{';
				str[offset++]='}';
				if(curr_node==root_node)
					return offset;				
				curr_node=json_get_father(curr_node);
				break;
			}
			if(curr_node->comp_no==0)
			{
				str[offset++]='{';
				child_node=json_get_first_child(curr_node);
				if(child_node==NULL)
					return -EINVAL;
				curr_node->comp_no++;
			}
			else if(curr_node->comp_no==curr_node->elem_no)
			{
				str[offset++]='}';
				if(curr_node==root_node)
					return offset;				
				curr_node=json_get_father(curr_node);
				break;
			}
			else
			{
				child_node=json_get_next_child(curr_node);
				if(child_node==NULL)
					return -EINVAL;
				str[offset++]=',';	
				curr_node->comp_no++;
			}

			str[offset++]='"';
			len=Strnlen(child_node->name,DIGEST_SIZE);
			Memcpy(str+offset,child_node->name,len);
			offset+=len;
			str[offset++]='"';
			str[offset++]=':';
			
			switch(json_get_type(child_node))
			{
				case JSON_ELEM_MAP:
				case JSON_ELEM_ARRAY:
					curr_node=child_node;
					curr_node->comp_no=0;
					break;
				case JSON_ELEM_STRING:
					str[offset++]='"';
					len=Strnlen(child_node->value_str,DIGEST_SIZE*16);
					Memcpy(str+offset,child_node->value_str,len);
					offset+=len;
					str[offset++]='"';
					break;
					
				case JSON_ELEM_NUM:
					len=Itoa(child_node->value,str+offset);
					offset+=len;
					break;
					
				case JSON_ELEM_BOOL:
					if(child_node->value)
					{
						Memcpy(str+offset,"true",4);
						offset+=4;
					}	
					else
					{
						Memcpy(str+offset,"false",5);
						offset+=5;
					}
					break;
				default:
					return -EINVAL;
			}	
			// at the beginning 
			break;
		case JSON_ELEM_ARRAY:
			if(curr_node->elem_no==0)
			{
				str[offset++]='[';
				str[offset++]=']';
				if(curr_node==root_node)
					return offset;				
				curr_node=json_get_father(curr_node);
				break;
			}
			if(curr_node->comp_no==0)
			{
				str[offset++]='[';
				child_node=json_get_first_child(curr_node);
				if(child_node==NULL)
					return -EINVAL;
				curr_node->comp_no++;
			}
			else if(curr_node->comp_no==curr_node->elem_no)
			{
				str[offset++]=']';
				if(curr_node==root_node)
					return offset;				
				curr_node=json_get_father(curr_node);
				break;
			}
			else
			{
				child_node=json_get_next_child(curr_node);
				if(child_node==NULL)
					return -EINVAL;
				str[offset++]=',';	
				curr_node->comp_no++;
			}

			switch(json_get_type(child_node))
			{
				case JSON_ELEM_MAP:
				case JSON_ELEM_ARRAY:
					curr_node=child_node;
					curr_node->comp_no=0;
					break;
				case JSON_ELEM_STRING:
					str[offset++]='"';
					len=Strnlen(child_node->value_str,DIGEST_SIZE*16);
					Memcpy(str+offset,child_node->value_str,len);
					offset+=len;
					str[offset++]='"';
					break;
					
				case JSON_ELEM_NUM:
					len=Itoa(child_node->value,str+offset);
					offset+=len;
					break;
					
				case JSON_ELEM_BOOL:
					if(child_node->value)
					{
						Memcpy(str+offset,"true",4);
						offset+=4;
					}	
					else
					{
						Memcpy(str+offset,"false",5);
						offset+=5;
					}
					break;
				default:
					return -EINVAL;
			}	
			// at the beginning 
			break;
		default:
			return -EINVAL;
	}

    }while(1);
  
    return offset; 
    
}

int  json_node_set_no(void * node,int no)
{
	JSON_NODE * json_node=node;
	if(node==NULL)
		return -EINVAL;
	json_node->comp_no=no;
	return 0;
}
int  json_node_get_no(void * node)
{
	JSON_NODE * json_node=node;
	if(node==NULL)
		return -EINVAL;
	return json_node->comp_no;
}

int  json_node_set_pointer(void * node,void * pointer)
{
	JSON_NODE * json_node=node;
	if(node==NULL)
		return -EINVAL;
	json_node->comp_pointer=pointer;
	return 0;
}
void * json_node_get_pointer(void * node)
{
	JSON_NODE * json_node=node;
	if(node==NULL)
		return NULL;
	return json_node->comp_pointer;
}

int json_node_getvalue(void * node,void * value, int max_len)
{
	JSON_NODE * json_node=node;
	int len;
	if(node==NULL)
		return -EINVAL;
	Memset(value,0,max_len);
	switch(json_node->elem_type)
	{
		case JSON_ELEM_NULL:
			return 0;
		case JSON_ELEM_NUM:
			if(max_len>=sizeof(long long))
			{
				len=sizeof(long long);
				*(long long *)value = json_node->value;
			}
			else if (max_len>=sizeof(int))
			{
				len=sizeof(int);
				*(int *)value = json_node->value;
			}
			else if (max_len>=sizeof(short))
			{
				len=sizeof(short);
				*(short *)value = json_node->value;
			}
			else if (max_len>=sizeof(char))
			{
				len=sizeof(char);
				*(char *)value = json_node->value;
			}
			break;
		case JSON_ELEM_BOOL:
			if (max_len>=sizeof(int))
			{
				len=sizeof(int);
				*(int *)value = json_node->value;
			}
			else if (max_len>=sizeof(short))
			{
				len=sizeof(short);
				*(short *)value = json_node->value;
			}
			else if (max_len>=sizeof(char))
			{
				len=sizeof(char);
				*(char *)value = json_node->value;
			}
			break;
		case JSON_ELEM_STRING:
			if(json_node->value_str==NULL)
			{
				len=0;
				break;
			}
			len=Strnlen(json_node->value_str,max_len);
			Memcpy(value,json_node->value_str,len);
			break;
		case JSON_ELEM_MAP:
		case JSON_ELEM_ARRAY:
		default:
			return -EINVAL;
	}
	return len;
}

int json_node_getname(void * node,char * name)
{
	JSON_NODE * json_node=node;
	int len;
	if(node==NULL)
		return -EINVAL;
	if(json_node->name==NULL)
		return 0;
	len=Strnlen(json_node->name,DIGEST_SIZE);
	Memcpy(name,json_node->name,len);
	if(len<DIGEST_SIZE)
		name[len]=0;
	return len;
}
