#include "jack_interface.h"
#include <jack/jack.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

/* Written by Christian Vogel <vogelchr@vogel.cx> in 2015. This program
is placed in the public domain to the largest extent possible, I do
not claim any rights to this code. */

static jack_interface_cb_ptr jack_interface_cb;
static jack_client_t *jack_interface_client=NULL;
static volatile unsigned long  jack_interface_callback_ctr=0;

/* we have one input, and one output port */
jack_port_t *ports[2];

static int
jack_interface_callback(jack_nframes_t nframes, void *arg)
{
	const float *inbuf;
	float *outbuf;

	(void) nframes;
	(void) arg;

	inbuf = jack_port_get_buffer(ports[0], nframes);
	outbuf = jack_port_get_buffer(ports[1], nframes);

	if (!jack_interface_cb)
		bzero(outbuf, sizeof(float)*nframes);
	else
		jack_interface_cb(nframes, inbuf, outbuf);

	jack_interface_callback_ctr += nframes;

	return 0;
}

void
jack_interface_set_cb(jack_interface_cb_ptr cb)
{
	jack_interface_cb = cb;
}

static void
jack_interface_shutdown_cb(void *arg)
{
	fprintf(stderr,"JACK has shut down!\n");
	jack_interface_client=NULL;
	(void) arg;
}

#if 0

static int
jack_interface_connect_port_to_phys_io(jack_port_t *our_port)
{
	const char **jack_ports;
	const char *inport_name, *outport_name;
	int i, match_flags;

	if (jack_port_flags(our_port) & JackPortIsInput)
		match_flags = JackPortIsOutput|JackPortIsPhysical;
	else
		match_flags = JackPortIsInput|JackPortIsPhysical;

	/* get all physical ports present that are the other direction */
	jack_ports = jack_get_ports(jack_interface_client,
		NULL, JACK_DEFAULT_AUDIO_TYPE,match_flags);

	if (!jack_ports || !jack_ports[0]) {
		fprintf(stderr,"Cannot lookup JACK ports.\n");
		goto error_out;
	}

	if (jack_port_flags(our_port) & JackPortIsInput) {
		inport_name = jack_port_name(our_port);
		outport_name = jack_ports[0];
	} else {
		inport_name = jack_ports[0];
		outport_name = jack_port_name(our_port);
	}

	if ((i=jack_connect(jack_interface_client,outport_name, inport_name)))
	{
		fprintf(stderr, "Cannot connect out port %s to in port %s.\n",
			outport_name, inport_name);
		fprintf(stderr, "Return value from jack_connect is %d.\n",i);
		goto error_out;
	}

	jack_free(jack_ports);
	return 0;

error_out:
	if (jack_ports)
		jack_free(jack_ports);
	return -1;
}

#endif

int
jack_interface_connect(char *clt_name, char *srv_name) {
	jack_status_t status;
	jack_options_t options = JackNullOption;
	char *clt_name_base;

	clt_name_base = basename(clt_name);
	if (srv_name)
		options |= JackServerName;

	jack_interface_client=jack_client_open(clt_name_base, options,
		&status, srv_name);

	if (jack_interface_client == NULL) {
		fprintf(stderr, "Unable to connect to JACK server.\n");
		return -1;
	}

	/* create one input, and one output port */
	if (!(ports[0] = jack_port_register(jack_interface_client, "input",
		JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)))
	{
		fprintf(stderr,"Unable to register input port with JACK.");
		goto error_out;
	}

	if (!(ports[1] = jack_port_register(jack_interface_client, "output",
		JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)))
	{
		fprintf(stderr,"Unable to register output port with JACK.\n");
		goto error_out;
	}

	/* start our audio processing, will happen in a separate
	   realtime thread */
	if (jack_set_process_callback(jack_interface_client,
		jack_interface_callback, 0 ) != 0)
	{
		fprintf(stderr,"Unable to register callback function with JACK.\n");
		goto error_out;
	}

	jack_on_shutdown(jack_interface_client,jack_interface_shutdown_cb,0);

	if (jack_activate(jack_interface_client)) {
		fprintf(stderr,"Unable to activate JACK client.\n");
		goto error_out;
	}

#if 0
	/* connect to first physical jack input/output */
	if (jack_interface_connect_port_to_phys_io(ports[0]))
		goto error_out;

	if (jack_interface_connect_port_to_phys_io(ports[1]))
		goto error_out;
#endif
	return 0;
error_out:
	if (jack_interface_client)
		jack_client_close(jack_interface_client);
	jack_interface_client=NULL;
	return -1;
}

int jack_interface_is_alive(unsigned long *ctr)
{
	if (ctr)
		*ctr = jack_interface_callback_ctr;
	return !!jack_interface_client;
}

unsigned long
jack_interface_framerate()
{
	if(!jack_interface_client)
		return 0;
	return jack_get_sample_rate(jack_interface_client);
}
