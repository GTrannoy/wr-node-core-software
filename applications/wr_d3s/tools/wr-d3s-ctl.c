/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <libwrnc.h>

#include "wr-d3s-common.h"
#include "wrtd-serializers.h"

struct wrnc_dev *wrnc ;

void help()
{
	fprintf(stderr, "wrtd-ping [options]\n");
	fprintf(stderr, "  -h             print this help\n");
	fprintf(stderr, "  -D 0x<dev_id>  device id\n");
	fprintf(stderr, "  -C command parameters provice a command & its parameters\n");
}

#define HMQ_TIMEOUT 500

static inline int send_and_receive_sync(struct wrnc_msg *msg)
{
	struct wrnc_hmq *hmq;
	int err;

	hmq = wrnc_hmq_open(wrnc, WR_D3S_IN_CONTROL, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send the message and get answer */
        err = wrnc_hmq_send_and_receive_sync(hmq, WR_D3S_OUT_CONTROL, msg,
					     HMQ_TIMEOUT);
	wrnc_hmq_close(hmq);

	return err;
}

int validate_acknowledge(struct wrnc_msg *msg)
{
	if (msg->datalen != 2 || msg->data[0] != WR_D3S_REP_ACK_ID) {
		return -1;
	}

	return 0;
}

void cmd_ping( int argc, char *argv[] )
{
    fprintf(stderr,"Ping... ");

    struct wrnc_msg msg = wrnc_msg_init(3); /* FIXME cannot use 2 */
    uint32_t id, seq = 0;
    int err;

    id = WR_D3S_CMD_PING;
    wrnc_msg_header(&msg, &id, &seq);

	/* Send the message and get answer */
    err = send_and_receive_sync(&msg);
        if (err) {
		fprintf(stderr,"timeout!\n");
		return ;
	}

 	if( !validate_acknowledge(&msg) )
 		fprintf(stderr,"OK!\n");
 	else
 		fprintf(stderr,"bad response!\n");


}

void cmd_pll_response( int argc, char *argv[] )
{
    int n_samples = atoi(argv[1]);
    char *filename = argv[2];
    fprintf(stderr,"Recording PLL response (%d samples) to '%s'... ", n_samples, filename);

    struct wrnc_msg msg = wrnc_msg_init(10); /* FIXME cannot use 2 */
    uint32_t id, seq = 0;
    int err;

    id = WR_D3S_CMD_START_RESPONSE_LOGGING;

    wrnc_msg_header(&msg, &id, &seq);
    wrnc_msg_int32(&msg, &n_samples);

    struct wrnc_hmq *hmq;

    send_and_receive_sync(&msg);

    FILE *f_out =fopen(filename,"wb");

    hmq = wrnc_hmq_open(wrnc, WR_D3S_OUT_CONTROL, WRNC_HMQ_OUTCOMING);
    if (!hmq)
	return ;

	int n = 0;
    while(n_samples > 0)
    {
    	int count, block_index;
    	struct wrnc_msg *resp = wrnc_hmq_receive(hmq);
    	if(resp)
    	{
    		wrnc_msg_header(resp, &id, &seq );
    		wrnc_msg_int32( resp, &count );
    		wrnc_msg_int32 (resp, &block_index);
    	
    		//printf("got : %d %d\n", count, block_index);
    		n_samples -= count / 2;
    		int i;

    		for(i = 0; i < count / 2; i++)
    		{
    			int phase, y;
    			wrnc_msg_int32( resp, &phase );
    			wrnc_msg_int32 (resp, &y);
	
			fprintf(stderr, "%d %d %d\n", n, phase, y);
    	
    			fprintf(f_out, "%d %d %d\n", n++, phase, y);
    		}
    	}
    }
    fclose(f_out);
    fprintf(stderr,"done!\n");

    wrnc_hmq_close(hmq);
}

int d3s_stream_config( int mode, int stream_id, double center_freq )
{
	double dds_freq = center_freq / 8.0;
	const double sample_rate = 500e6;
	uint64_t tune = (uint64_t) ( (double)(1LL<<42) * (dds_freq / sample_rate) * 8.0 );

	uint32_t tune_hi = (tune >> 32) & 0xffffffff;
	uint32_t tune_lo = (tune >> 0) & 0xffffffff;

	printf("HI=0x%x\n", tune_hi);
    	printf("LO=0x%x\n", tune_lo);

   	struct wrnc_msg msg = wrnc_msg_init(10); /* FIXME cannot use 2 */
    	uint32_t id, seq = 0;
    	int err;

    	id = WR_D3S_CMD_STREAM_CONFIG;

    	wrnc_msg_header(&msg, &id, &seq);
    	wrnc_msg_int32(&msg, &mode);
	wrnc_msg_int32(&msg, &stream_id);
	wrnc_msg_int64(&msg, &tune);
	
    	send_and_receive_sync(&msg);

	return validate_acknowledge(&msg);
}

void cmd_stream_config( int argc, char *argv[] )
{
	int mode;

	if(argc < 1)
		return;

	if(!strcmp(argv[1],"master"))
	{
		mode = D3S_STREAM_MASTER;
	}
	else if(!strcmp(argv[1],"master"))
	{
		mode = D3S_STREAM_SLAVE;
	} if(!strcmp(argv[1],"off"))
	{
		fprintf(stderr,"RF streaming: disabled.\n");
		mode = D3S_STREAM_OFF;
		d3s_stream_config( mode, 0, 0 );
		return;
	}


	if(argc >= 3)
	{
		int stream_id = atoi(argv[2]);
		double center_freq = atof(argv[3]);
		fprintf(stderr,"RF streaming: mode %s, stream_id = %d, center_freq = %.6f MHz\n", mode == D3S_STREAM_SLAVE?"slave":"master",stream_id, center_freq);
		d3s_stream_config( mode, stream_id, center_freq );
	}


}

int main(int argc, char *argv[])
{
	uint32_t dev_id = 0, n = 1;
	uint64_t period = 0;
	char c;

	char *cmd = NULL;
	char **cmd_params;
	int cmd_params_count;

	while ((c = getopt (argc, argv, "hD:n:p:C:")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			help();
			exit(1);
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 'n':
			sscanf(optarg, "%d", &n);
			break;
		case 'p':
			sscanf(optarg, "%"SCNu64, &period);
			break;

		case 'C':
			cmd = strdup(optarg);
			cmd_params = &argv[optind-1];
			cmd_params_count = argc - optind;
			break;
		}
	}

	if (dev_id == 0) {
		help();
		exit(1);
	}

//	atexit(wrtd_exit);

	wrnc_init();

	wrnc = wrnc_open_by_fmc(dev_id);

	if(!wrnc)
	{
	    fprintf(stderr, "can't open device...\n");
	    return -1;

	}

	if(cmd)
	{
	    if(!strcasecmp(cmd,"ping"))
		cmd_ping(cmd_params_count, cmd_params);
	    if(!strcasecmp(cmd,"pll_response"))
		cmd_pll_response(cmd_params_count, cmd_params);
	    if(!strcasecmp(cmd,"stream"))
		cmd_stream_config(cmd_params_count, cmd_params);
	}

	wrnc_close(wrnc);
	exit(0);
}
