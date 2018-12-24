#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"
#include "consdrv.h"

#define CONS_BUFFER_SIZE 24

static struct consreg {
	kz_thread_id_t id;
	int index;

	char *send_buf;
	char *recv_buf;
	int send_len;
	int recv_len;

	long dummy[3];
} consreg[CONSDRV_DEVICE_NUM];

static void send_char(struct consreg *cons){
	int i;
	serial_send_byte(cons->index, cons->send_buf[0]);
	cons->send_len--;
	for(i=0;i<cons->send_len;i++){
		cons->send_buf[i] = cons->send_buf[i+1];
	}
}

static void send_string(struct consreg *cons, char *str, int len){
	int i;
	for(i=0;i<len;i++){
		if(str[i] == '\n')
			cons->send_buf[cons->send_len++] = '\r';
		cons->send_buf[cons->send_len++] = str[i];
	}
	if(cons->send_len && !serial_intr_is_send_enable(cons->index)){
		serial_intr_send_enable(cons->index);
		send_char(cons);
	}
}

static int consdrv_intrproc(struct consreg *cons){
	unsigned char c;
	char *p;

	if(serial_is_recv_enable(cons->index)){
		c = serial_recv_byte(cons->index);
		if(c == '\r')
			c = '\n';
		send_string(cons, &c, 1);

		if(cons->id){
			if(c != '\n'){
				cons->recv_buf[cons0>recv_len++] = c;
			} else {
				p = kx_kmalloc(CONS_BUFFER_SIZE);
				memcpy(p, cons->recv_buf, cons->recv_len);
				kx_send(KSGBOX_ID_CONSINPUT, cons->recv_len, p);
				cons->recv_len = 0;
			}
		}
	}

	if(serial_is_send_enable(cons->index)){
		if(!cons->id || !cons->send_len){
			serial_intr_send_disable(cons->index);
		} else {
			send_char(cons);
		}
	}

	return 0;
}

