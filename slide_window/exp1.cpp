#include "sysinclude.h"

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4

typedef enum {data,ack,nak} frame_kind;

struct frame_head
{
	      frame_kind       kind;	//帧类型
          unsigned int     seq;		//序号
          unsigned int     ack;		//确认号
          unsigned char data[100];	//数据
};

struct frame
{
	frame_head head;
	unsigned int size;
};
/*
* 1 bit protocol variable declaration
*/
unsigned int next_frame_to_send = 0;
unsigned int frame_expect_to_ack = 0;
char buffer[256][256];
unsigned int buffer_size[256];
unsigned int buffer_seq[256];
/*
* 停等协议测试函数
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	switch(messageType){
		case MSG_TYPE_TIMEOUT:{
			SendFRAMEPacket((unsigned char*)buffer[frame_expect_to_ack], buffer_size[frame_expect_to_ack]); //如果超时就重传当前缓存的帧
			return 0;
		}
		case MSG_TYPE_SEND:{
			memcpy(buffer[next_frame_to_send], pBuffer, bufferSize);
			buffer_size[next_frame_to_send] = bufferSize;
			buffer_seq[next_frame_to_send] = ((frame*)pBuffer)->head.seq;
			if (next_frame_to_send == frame_expect_to_ack) SendFRAMEPacket((unsigned char*)pBuffer, bufferSize);
			next_frame_to_send++;							
			return 0;
		}
		case MSG_TYPE_RECEIVE:{			 
			if (((frame*)pBuffer)->head.ack == buffer_seq[frame_expect_to_ack]){
			 	frame_expect_to_ack++;
			 	if (frame_expect_to_ack < next_frame_to_send)
			 		SendFRAMEPacket((unsigned char*)buffer[frame_expect_to_ack], buffer_size[frame_expect_to_ack]);			 
				return 0;
			}
		}
	}
	return -1;
}

/*
* n bit protocol variable declaration
*/
unsigned int buffered_upperbound = 0;
unsigned int n_next_frame_to_send = 0;
unsigned int n_frame_expect_to_ack = 0;
char nBuffer[256][256];
unsigned int n_buffer_size[256];
unsigned int n_buffer_seq[256];
/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	switch(messageType){
		case MSG_TYPE_TIMEOUT:{
			for (int i=n_frame_expect_to_ack; i<n_next_frame_to_send; i++)
				SendFRAMEPacket((unsigned char*)nBuffer[i], n_buffer_size[i]); 
			return 0;
		}
		case MSG_TYPE_SEND:{
			memcpy(nBuffer[buffered_upperbound], pBuffer, bufferSize);
			n_buffer_size[buffered_upperbound] = bufferSize;
			n_buffer_seq[buffered_upperbound] = ntohl(((frame*)pBuffer)->head.seq);
			buffered_upperbound++;
			while (n_next_frame_to_send < n_frame_expect_to_ack + WINDOW_SIZE_BACK_N_FRAME && n_next_frame_to_send < buffered_upperbound){ 
				SendFRAMEPacket((unsigned char*)nBuffer[n_next_frame_to_send], n_buffer_size[n_next_frame_to_send]);
				n_next_frame_to_send++;							
			}
			return 0;
		}
		case MSG_TYPE_RECEIVE:{			 
			while (ntohl(((frame*)pBuffer)->head.ack) >= n_buffer_seq[n_frame_expect_to_ack])
			 	n_frame_expect_to_ack++;
			while (n_next_frame_to_send < n_frame_expect_to_ack + WINDOW_SIZE_BACK_N_FRAME && n_next_frame_to_send < buffered_upperbound){ 
					SendFRAMEPacket((unsigned char*)nBuffer[n_next_frame_to_send], n_buffer_size[n_next_frame_to_send]);
					n_next_frame_to_send++;							
			}							
			return 0;
		}
	}
	return -1;
}

/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	return 0;
}

