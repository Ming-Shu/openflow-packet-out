#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include"channel_communication.h"
//#include "flow.h"
#include "oxm_match.h"
#include "proxy_table.h"
#include "packet_out.h"
extern struct virtual_onu_olt_port *pOnutab;

void Add_action_output(struct ofp_action_header **action,int value)
{

  struct ofp_action_output *output;

  output =*(struct ofp_action_output**)action;
  output->type = htons(OFPAT_OUTPUT);     
  output->len = htons(sizeof(struct ofp_action_output));       
  output->port =htonl(value);
  output->max_len =htons(0);
  *action = (struct ofp_action_output *)output;
}//Add_action_output

void Add_action_set_field(struct ofp_action_header **action,int value)
{
  struct ofp_action_set_field *set_field;
  uint32_t *w_oxm_action_field;
  uint16_t *w_2byte_oxm_tlv;
  uint8_t  oxm_len,*pOxm_tlv,*least_oxm_address;

  set_field =*(struct ofp_action_set_field **)action;
  set_field->type = htons(OFPAT_SET_FIELD); 
  w_oxm_action_field = pOxm_tlv = &set_field->field;
  *w_oxm_action_field =  ntohl(OXM_OF_VLAN_VID);
  w_2byte_oxm_tlv= (w_oxm_action_field+1);//w_oxm_action_field+1 =oxm_tlv_header memory 4byte
  *(w_2byte_oxm_tlv)=ntohs(value); 
  least_oxm_address=w_2byte_oxm_tlv+1;
  oxm_len = OXM_LENGTH(UNPACK_OXM_TLV(*pOxm_tlv,*(pOxm_tlv+1),*(pOxm_tlv+2),*(pOxm_tlv+3)));
  set_field->len = htons(sizeof(struct ofp_action_set_field))+htons(OFP_ACTION_SET_FIELD_OXM_PADDING(oxm_len+4))+htons(oxm_len);//oxm_tlv_field 4byte
  
/*
printf("Proxy_set_field->len%d\n\n",htons(set_field->len));
printf("Proxy_instruction->len:%d\n\n",htons(instruction->len));
printf("Proxy_action_header_length:%d\n\n",htons(flow_mod->header.length));*/

  *action= least_oxm_address+OFP_ACTION_SET_FIELD_OXM_PADDING(oxm_len+4);//next pointer address
}//Add_action_set_field

void Add_action_push(struct ofp_action_header **action,enum ofp_action_type type, uint16_t ethertype)
{

  struct ofp_action_push *push;

  push =*(struct ofp_action_push **)action;
  push->type = htons(type);     
  push->len = htons(sizeof(struct ofp_action_push));       
  push->ethertype = htons(ethertype); 
/*
printf("Flow_instruction_push->len:%d\n\n",htons(instruction->len));
*/
  *action= (struct ofp_action_header *)(push+1) ;//next pointer address
}//Add_action_push

void outport_covert(int *outport,int *use_vlan,struct virtual_onu_olt_port *qurey)//covert from virtual_port
{
    printf("---------outport_covert ------------\n\n" );
    	int out_port,new_out_port;
	int a;
	struct virtual_onu_olt_port *p;
	out_port  = *outport;
	if(out_port>ONU_PORT_NUM){
		new_out_port=of_init_port(out_port);
		*use_vlan = -1;
		printf("out_port:%d,new_out_port:%d\n\n",out_port,new_out_port);
	
	}	
	if(out_port<=ONU_PORT_NUM){/*The in_port from ONU*/
		p = qurey;
		for(a=0;a<ONU_PORT_NUM;a++,p++){
			if(p->vir_port==out_port){
				printf("out_port:%d,olt_of_port:%d,used_vlan:%d\n\n",out_port,p->olt_of_port,p->used_vlan);
				*use_vlan = p->used_vlan;
				new_out_port=p->olt_of_port;

				break;
			}
		}
	} 
	*outport = new_out_port;
}//outport_covert

void Actions_type_cache(struct ofp_action_header **action_header,uint16_t len,enum ofp_action_type type)
{
printf("----------------------actions_type_address -----------------------\n\n");
	struct ofp_action_header *action;
	action = *action_header;
	while(len>0&&(htons(action->type)!=type))
	{
	  len=len-(action->len);	
	  action=action+(htons(action->len)/8);
	}
	*action_header=action;	
}//Actions_type_cache

int read_action_header( struct ofp_action_header *action,enum ofp_action_type type)
{
  int value;

	Actions_type_cache(&action,action->len,type);
	switch(type){
		case OFPAT_OUTPUT:{
				printf("Action header that is 'OFPAT_OUTPUT'\n\n" );
				struct ofp_action_output *p;
				p = (struct ofp_action_output *)action;
				value = htonl(p->port);
				break;
		}
		case OFPAT_PUSH_VLAN:{
				printf("Action header that is 'OFPAT_PUSH_VLAN'\n\n" );
				struct ofp_action_push *p;
				p = (struct ofp_action_push*)action;
				value = htons(p->ethertype);
				break;
		}
		default:{
			printf("Action header that is not exist!\n" );
			break;
		}	
	}//switch

  return value;		
}//read_action_header

void packet_out_handle(char* buffer,int buf_len,int sw_sockfd)
{
  printf("------------------Staring handle 'packet_out' message from controller-------------\n\n");
  int in_port,out_port,vlan,packet_out_length,actions_len;
  struct ofp_packet_out *p,*c;
  struct ofp_action_header *action_header;
  p = (struct ofp_packet_out*)buffer;
  packet_out_length = htons(p->header.length);
  in_port = htonl(p->in_port);
  actions_len = htons(p->actions_len);
  out_port = read_action_header(p->actions,OFPAT_OUTPUT);
  printf("The init_info packet_out_length:%d, in_port :%d\n ,actions_len:%d ,out_port:%d\n\n",packet_out_length,in_port,actions_len,out_port);
  outport_covert(&out_port,&vlan,pOnutab); 
  printf("The outport_covert:%d,vlan:%d\n",out_port,vlan);
  
  action_header = p->actions;
  if(in_port ==OFPP_CONTROLLER){
     printf("in_port is controller-------------\n\n");
     Add_action_push(&action_header,OFPAT_PUSH_VLAN,0x8100);
     p->actions_len = htons(action_header->len);
     
     Add_action_set_field(&action_header,vlan);
     p->actions_len = htons(action_header->len);
     
     Add_action_output(&action_header,out_port);
     p->actions_len = htons(action_header->len);     
     p->header.length =   packet_out_length -  actions_len +  htons(p->actions_len);
     packet_out_length = htons(p->header.length);
     
     printf("The init_info packet_out_length:%d ,actions_len:%d \n\n",packet_out_length,actions_len); 
  }else if(in_port>ONU_PORT_NUM){
     printf("virtual in_port is OFSW (%d)------------\n\n",in_port);      
  }else if(in_port <= ONU_PORT_NUM && in_port>0){
     printf("virtual in_port is ONU (%d)------------\n\n",in_port);  
  }else{           
  }//else               
  
  send(sw_sockfd,p,packet_out_length,0);
 
}//packet_out_handle*/

