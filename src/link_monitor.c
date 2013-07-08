#include "link_monitor.h"

void link_monitor_ping() {
	request_phone_state();
}

void link_monitor_handle_failure(int error, struct Data* d) {
	if(d->link_status == LinkStatusOK)
	{	
		//switch(error) {
		//	case APP_MSG_BUSY:
		//		return; //test
		//	case APP_MSG_SEND_TIMEOUT:
		//		return;
		//	default:
		//		return; //vibes_short_pulse();
		//}
		d->link_status = LinkStatusFailed;
	}
}

void link_monitor_handle_success(struct Data* d)
{
	if(d->link_status == LinkStatusFailed)
	{
		//Notify the user of reconnection
		//const uint32_t seg[] = { 30, 30, 30};	
		//VibePattern pattern = {.durations =  seg,.num_segments = ARRAY_LENGTH(seg)};
		//vibes_enqueue_custom_pattern(pattern);
	    d->link_status = LinkStatusOK;
	}
}