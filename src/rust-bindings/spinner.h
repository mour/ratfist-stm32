#ifndef SPINNER_H_
#define SPINNER_H_

#include <mouros/mailbox.h>

extern mailbox_t *SPINNER_MSG_QUEUE_PTR;

void *spinner_get_context();
void spinner_comm_loop(void *context);

#endif

