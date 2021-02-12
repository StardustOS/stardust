#include <os/xenstore.h>
#include <public/event_channel.h>
#include <public/sched.h>
#include <public/io/xs_wire.h>

static evtchn_port_t xenstore_evt;
extern char _text;
struct xenstore_domain_interface * xenstore;

int xenstore_init(start_info_t * start)
{
	xenstore = (struct xenstore_domain_interface*)
		((machine_to_phys_mapping[start->store_mfn] << 12)
		 +
		((unsigned long)&_text));
	xenstore_evt = start->store_evtchn;
	return 0;
}

int xenstore_write_request(char * message, int length)
{
	if(length > XENSTORE_RING_SIZE) return -1;

	int i;
	for(i=xenstore->req_prod ; length > 0 ; i++,length--)
	{
		XENSTORE_RING_IDX data;
		do
		{
			data = i - xenstore->req_cons;
			mb();
		} while (data >= sizeof(xenstore->req));
		int ring_index = MASK_XENSTORE_IDX(i);
		xenstore->req[ring_index] = *message;
		message++;
	}
	wmb();
	xenstore->req_prod = i;
	return 0;
}

int xenstore_read_response(char * message, int length)
{
	int i;
	for(i=xenstore->rsp_cons ; length > 0 ; i++,length--)
	{
		XENSTORE_RING_IDX data;
		do
		{

			data = xenstore->rsp_prod - i;
			mb();
		} while (data == 0);
		int ring_index = MASK_XENSTORE_IDX(i);
		*message = xenstore->rsp[ring_index];
		message++;
	}
	xenstore->rsp_cons = i;
	return 0;
}

static int req_id = 0;

#define NOTIFY() \
	do {\
	   	struct evtchn_send event;\
	    event.port = xenstore_evt;\
		HYPERVISOR_event_channel_op(EVTCHNOP_send, &event);\
	} while(0)

#define IGNORE(n) \
	do {\
		char buffer[XENSTORE_RING_SIZE];\
		xenstore_read_response(buffer, n);\
	} while(0)

int xenstore_write(char * key, char * value)
{
	int key_length = strlen(key);
	int value_length = strlen(value);
	struct xsd_sockmsg msg;
	msg.type = XS_WRITE;
	msg.req_id = req_id;
	msg.tx_id = 0; 
	msg.len = 2 + key_length + value_length;
	xenstore_write_request((char*)&msg, sizeof(msg));
	xenstore_write_request(key, key_length + 1);
	xenstore_write_request(value, value_length + 1);
	NOTIFY();
	xenstore_read_response((char*)&msg, sizeof(msg));
	IGNORE(msg.len);
	if(msg.req_id != req_id++)
	{
		return -1;
	}
	return 0;
}

int xenstore_read(char * key, char * value, int value_length)
{
	int key_length = strlen(key);
	struct xsd_sockmsg msg;
	msg.type = XS_READ;
	msg.req_id = req_id;
	msg.tx_id = 0; 
	msg.len = 1 + key_length;

	xenstore_write_request((char*)&msg, sizeof(msg));
	xenstore_write_request(key, key_length + 1);

	NOTIFY();
	xenstore_read_response((char*)&msg, sizeof(msg));
	if(msg.req_id != req_id++)
	{
		IGNORE(msg.len);
		return -1;
	}

	if(value_length >= msg.len)
	{
		xenstore_read_response(value, msg.len);
		return 0;
	}
	xenstore_read_response(value, value_length);
	IGNORE(msg.len - value_length);
	return -2;
}

int xenstore_ls(char * key, char * values, int value_length)
{
	int key_length = strlen(key);
	struct xsd_sockmsg msg;
	msg.type = XS_DIRECTORY;
	msg.req_id = req_id;
	msg.tx_id = 0; 
	msg.len = 1 + key_length;
	/* Write the message */
	xenstore_write_request((char*)&msg, sizeof(msg));
	xenstore_write_request(key, key_length + 1);
	/* Notify the back end */
	NOTIFY();
	xenstore_read_response((char*)&msg, sizeof(msg));
	if(msg.req_id != req_id++)
	{
		IGNORE(msg.len);
		return -1;
	}
	/* If we have enough space in the buffer */
	if(value_length >= msg.len)
	{
		xenstore_read_response(values, msg.len);
		return msg.len;
	}
	/* Truncate */
	xenstore_read_response(values, value_length);
	IGNORE(msg.len - value_length);
	return -2;
}

int xenstore_read_domain_id() 
{
	char buffer[5];
	buffer[4] = '\0';
	xenstore_read("domid", buffer, 4);
	int i;
	sscanf(buffer, "%d", &i);
	return i;
}
