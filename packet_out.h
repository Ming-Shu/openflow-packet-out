#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openflow_1_3.h"

void Add_action_output(struct ofp_action_header **action,int value);
void Add_action_set_field(struct ofp_action_header **action,int value);
void Add_action_push(struct ofp_action_header **action,enum ofp_action_type type, uint16_t ethertype);
void outport_covert(int *outport,int *use_vlan,struct virtual_onu_olt_port *qurey);
void Actions_type_cache(struct ofp_action_header **action_header,uint16_t len,enum ofp_action_type type);
int read_action_header( struct ofp_action_header *action,enum ofp_action_type type);
void packet_out_handle(char* buffer,int buf_len,int sw_sockfd);
