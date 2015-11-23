#include "sysinclude.h"

extern void rip_sendIpPkt(unsigned char *pData, UINT16 len,unsigned short dstPort,UINT8 iNo);

extern struct stud_rip_route_node *g_rip_route_table;

struct route_node
{   
    char heaader[4]; //address family and route tag
    unsigned int dest;
    unsigned int mask;
    unsigned int nexthop;
    unsigned int metric;
};

struct rip_pac
{
    char command;
    char version;
    unsigned short zero;
    route_node route_table[25];    
};

int stud_rip_packet_recv(char *pBuffer,int bufferSize,UINT8 iNo,UINT32 srcAdd)
{	
    char command = pBuffer[0];
    char version  = pBuffer[1];  
    //Validation
    if (command != 1 && command != 2){
        ip_DiscardPkt(pBuffer, STUD_RIP_TEST_COMMAND_ERROR);
        return -1;
    }
    if (version != 2){ 
        ip_DiscardPkt(pBuffer, STUD_RIP_TEST_VERSION_ERROR);
        return -1;
    }
    //Handle Request and Response    
    if (command == 1){//request
        rip_pac* packet = new rip_pac;        
        packet->command = 2;
        packet->version = 2;
        packet->zero = 0;
        int loc = 0;
        stud_rip_route_node* current = g_rip_route_table;
        while (current){
            if (current->if_no != iNo){                
                packet->route_table[loc].heaader[0] = 0;
                packet->route_table[loc].heaader[1] = 2;
                packet->route_table[loc].heaader[2] = 0;
                packet->route_table[loc].heaader[3] = 0;
                packet->route_table[loc].dest = htonl(current->dest);
                packet->route_table[loc].mask = htonl(current->mask);
                packet->route_table[loc].nexthop = htonl(current->nexthop);
                packet->route_table[loc++].metric = htonl(current->metric);
            }
            current = current->next;
        }
        rip_sendIpPkt((unsigned char*)packet, loc*20+4, 520, iNo);
    }else if (command == 2){//response        
        for (int i=4; i<bufferSize; i+=20){           
            stud_rip_route_node* current = g_rip_route_table;
            bool flag = true;
            while (current){                
                if (ntohl(*((int*)(pBuffer+i+4)))==current->dest && 
                    ntohl(*((int*)(pBuffer+i+8)))==current->mask){
                    flag = false;
                    if (current->nexthop == ntohl(*((int*)(pBuffer+i+12)))){
                        current->metric = ntohl(*((int*)(pBuffer+i+16)))+1;
                        if (current->metric>=16) current->metric = 16;
                        current->nexthop = srcAdd;
                        current->if_no = iNo;
                    }
                    else{
                        int new_metirc = ntohl(*((int*)(pBuffer+i+16)))+1;
                        if (new_metirc <= current->metric){
                            current->metric = new_metirc;
                            current->nexthop = srcAdd;
                            current->if_no = iNo;
                        }
                        if (new_metirc >= 16) current->metric = 16;                        
                    }
                }
                current = current->next;
            }
            if (flag){ //route entry does not exist
                current = g_rip_route_table;
                while (current->next) current = current->next;
                if (ntohl(*((int*)(pBuffer+i+16)))+1 < 16){
                    stud_rip_route_node* new_node = new stud_rip_route_node;
                    new_node->dest = ntohl(*((int*)(pBuffer+i+4)));
                    new_node->mask = ntohl(*((int*)(pBuffer+i+8)));
                    new_node->nexthop = srcAdd;
                    new_node->metric = ntohl(*((int*)(pBuffer+i+16)))+1;
                    new_node->if_no = iNo;
                    new_node->next = NULL;
                    current->next = new_node;
                }
            }
        }
    }
	return 0;
}

void stud_rip_route_timeout(UINT32 destAdd, UINT32 mask, unsigned char msgType)
{
    stud_rip_route_node* current = g_rip_route_table;
    if (msgType == RIP_MSG_SEND_ROUTE){
        rip_pac** packet = new rip_pac*[2];
        packet[1] = new rip_pac;
        packet[2] = new rip_pac;        
        packet[1]->command = packet[2]->command = 2;
        packet[1]->version = packet[2]->version = 2;
        packet[1]->zero = packet[2]->zero = 0;
        int loc[3] = {0};
        stud_rip_route_node* current = g_rip_route_table;
        while (current){ 
            int des = 3-current->if_no;           
            packet[des]->route_table[loc[des]].heaader[0] = 0;
            packet[des]->route_table[loc[des]].heaader[1] = 2;
            packet[des]->route_table[loc[des]].heaader[2] = 0;
            packet[des]->route_table[loc[des]].heaader[3] = 0;
            packet[des]->route_table[loc[des]].dest = htonl(current->dest);
            packet[des]->route_table[loc[des]].mask = htonl(current->mask);
            packet[des]->route_table[loc[des]].nexthop = htonl(current->nexthop);
            packet[des]->route_table[loc[des]++].metric = htonl(current->metric);            
            current = current->next;
        }
        rip_sendIpPkt((unsigned char*)packet[1], loc[1]*20+4, 520, 1);
        rip_sendIpPkt((unsigned char*)packet[2], loc[2]*20+4, 520, 2);
    }	
    else{        
        while (current){
            if (current->dest == destAdd && current->mask == mask) current->metric = 16;
            current = current->next;
        }
    }	
}

