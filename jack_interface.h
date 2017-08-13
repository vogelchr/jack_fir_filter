#ifndef JACK_INTERFACE_H
#define JACK_INTERFACE_H

/* user callback for the jack interface, called with two buffers,
   one holding the input samples, the other holding the output sampes */

typedef int (*jack_interface_cb_ptr)(
	unsigned long, const float *, float *);

extern int jack_interface_is_alive(unsigned long *ctr);
extern int jack_interface_add_port(char *pn, int is_input);
extern int jack_interface_connect(char *clt_name, char *srv_name);
extern void jack_interface_set_cb(jack_interface_cb_ptr cb);

extern unsigned long jack_interface_framerate();

#endif
