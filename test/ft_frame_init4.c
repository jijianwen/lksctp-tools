/* SCTP kernel reference Implementation
 * (C) Copyright IBM Corp. 2001, 2003
 * Copyright (C) 1999 Cisco and Motorola
 * Copyright (c) Nokia, 2002
 *
 * This file is part of the SCTP kernel reference Implementation
 *
 * $Id: ft_frame_init4.c, 
 * 
 * This is Functional Test 4 for the SCTP kernel reference
 * implementation state machine.
 * 
 * Case Study 1: Initialization Collision
 * Scenario 4.  Two INITs are sent and cross. But one of them is lost
 *
 * Set up a link, send message from sk1 to sk2 and from sk2 to sk1  
 * simultaneously. This will cause overlapping INIT chunk. let 
 * Z->A INIT lost. Go through the setting up procedure step by step,
 * See the association is up and messages appear. Then go home.  
 * 
 * The SCTP reference implementation is free software; 
 * you can redistribute it and/or modify it under the terms of 
 * the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * The SCTP reference implementation is distributed in the hope that it 
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *                 ************************
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU CC; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 * 
 * Please send any bug reports or fixes you make to the
 * email address(es):
 *    lksctp developers <lksctp-developers@lists.sourceforge.net>
 * 
 * Or submit a bug report through the following website:
 *    http://www.sf.net/projects/lksctp
 *
 * Written or modified by: 
 *    La Monte H.P. Yarroll <piggy@acm.org>
 *    Narasimha Budihal     <narsi@refcode.org>
 *    Karl Knutson          <karl@athena.chicago.il.us>
 *    Jon "Taz" Mischo      <taz@refcode.org>
 *    Sridhar Samudrala     <samudrala@us.ibm.com>
 *    Dajiang Zhang         <dajiang.zhang@nokia.com>
 * 
 * Any bugs reported given to us we will try to fix... any fixes shared will
 * be incorporated into the next SCTP release.
 */

#include <linux/types.h>
#include <linux/list.h> /* For struct list_head */
#include <linux/socket.h>
#include <linux/ip.h>
#include <linux/time.h> /* For struct timeval */
#include <net/sock.h>
#include <linux/wait.h> /* For wait_queue_head_t */
#include <linux/skbuff.h>
#include <linux/errno.h>
#include <net/sctp/sctp.h>
#include <errno.h>
#include <funtest.h>

int
main(int argc, char *argv[])
{
        struct sock *sk1;
        struct sock *sk2;
        struct sockaddr_in loop1, loop2;	
        uint8_t *message01 = "First message from A!\n";
	uint8_t *message02 = "Second message from A!\n";
	uint8_t *message11 = "First message from Z!\n";
	uint8_t *message12 = "Second message from Z!\n";
	int error;	
        
        /* Do all that random stuff needed to make a sensible universe. */
	init_Internet();
        sctp_init();

        /* Create the two endpoints which will talk to each other.  */
        sk1 = sctp_socket(PF_INET, SOCK_SEQPACKET);
        sk2 = sctp_socket(PF_INET, SOCK_SEQPACKET);

        loop1.sin_family = AF_INET;
        loop1.sin_addr.s_addr = SCTP_IP_LOOPBACK;
        loop1.sin_port = htons(SCTP_TESTPORT_1);
	loop2.sin_family = AF_INET;
        loop2.sin_addr.s_addr = SCTP_IP_LOOPBACK;
	loop2.sin_port = htons(SCTP_TESTPORT_2);

        /* Bind these sockets to the test ports.  */        
        error = test_bind(sk1, (struct sockaddr *)&loop1, sizeof(loop1));
        if (error != 0) { DUMP_CORE; }        
        
        error = test_bind(sk2, (struct sockaddr *)&loop2, sizeof(loop2));
        if (error != 0) { DUMP_CORE; }
        
	/* Mark sk2 as being able to accept new associations. */
	if (0 != sctp_seqpacket_listen(sk2, 1)) {
		DUMP_CORE;
	}
	test_frame_send_message(sk1, (struct sockaddr *)&loop2, message01);
        

	/* Let ep2 send message to ep1 to cause INIT collision. */
	        
	/* Mark sk1 as being able to accept new associations. */
	if (0 != sctp_seqpacket_listen(sk1, 1)) {
		DUMP_CORE;
	}
        
	test_frame_send_message(sk2, (struct sockaddr *)&loop1, message11);
        
	/* Walk through the startup sequence.  */
        /* We should have a INIT sitting on the Internet. */
	if (!test_for_chunk(SCTP_CID_INIT, TEST_NETWORK0)) {
		DUMP_CORE;
	}
	printf("\n\n A->Z INIT! \n\n");

	/* Z->A INIT is lost. */
	if (test_step(SCTP_CID_INIT, TEST_NETWORK0) <= 0) {
		DUMP_CORE;
	}
	printf("\n\n Let Z->A INIT missing! \n\n");
	
	test_kill_next_packet(SCTP_CID_INIT);

	/* Next we expect a INIT ACK. */
	if (test_step(SCTP_CID_INIT_ACK, TEST_NETWORK0) <= 0) {
		DUMP_CORE;
	}
	printf("\n\n Z->A INIT_ACK! \n\n");
		
	/* We expect COOKIE ECHO and DATA.  */
	if (test_step(SCTP_CID_COOKIE_ECHO, TEST_NETWORK0) <= 0) {
		DUMP_CORE;
	}
	printf("\n\n A->Z COOKIE_ECHO! \n\n");

	if (!test_for_chunk(SCTP_CID_DATA, TEST_NETWORK0) ) {
		DUMP_CORE;
	}
	printf("\n\n A->Z DATA! \n\n");

	/* We expect COOKIE ACK and SACK.  */	
	if (test_step(SCTP_CID_COOKIE_ACK, TEST_NETWORK0) <=0 ) {
		DUMP_CORE;
	}
	printf("Z->A COOKIE_ACK ! \n\n");

	if (test_step(SCTP_CID_SACK, TEST_NETWORK0) <= 0) {
		DUMP_CORE;
	}       
	printf("\n\n Z->A SACK! \n\n");	

	error = test_run_network();
	if (error != 0) { DUMP_CORE; }

	test_frame_get_event(sk1, SCTP_ASSOC_CHANGE, SCTP_COMM_UP);
	test_frame_get_event(sk2, SCTP_ASSOC_CHANGE, SCTP_COMM_UP);

	test_frame_get_message(sk2, message01);

	/* Send message again. */

	printf("Send more messages! \n\n");
	test_frame_send_message(sk1, (struct sockaddr *)&loop2, message02);
        test_frame_send_message(sk2, (struct sockaddr *)&loop1, message12);

        error = test_run_network();
        if (error != 0) { DUMP_CORE; }

	test_frame_get_message(sk2, message02);
	test_frame_get_message(sk1, message11);	
	test_frame_get_message(sk1, message12);
	
	/* If we get to this point, the test has passed.  The rest is
	 * just clean-up.
	 */
	/* Shut down the link.  */
	sctp_close(sk1, /* timeout */ 0);

	error = test_run_network();
        if (error != 0) { DUMP_CORE; }	
	
        sctp_close(sk2, /* timeout */ 0);   
	
	if (0 == error) {
		printk("\n\n%s passed\n\n\n", argv[0]);
	}

        /* Indicate successful completion.  */
        exit(error);

} /* main() */

