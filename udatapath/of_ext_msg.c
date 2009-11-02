/* Copyright (c) 2009 The Board of Trustees of The Leland Stanford
 * Junior University
 * 
 * We are making the OpenFlow specification and associated documentation
 * (Software) available for public use and benefit with the expectation
 * that others will use, modify and enhance the Software and contribute
 * those enhancements back to the community. However, since we would
 * like to make the Software available for broadest use, with as few
 * restrictions as possible permission is hereby granted, free of
 * charge, to any person obtaining a copy of this Software to deal in
 * the Software under the copyrights without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * The name and trademarks of copyright holder(s) may NOT be used in
 * advertising or publicity pertaining to the Software or any
 * derivatives without specific, written prior permission.
 */

#include <errno.h>
#include <arpa/inet.h>
#include "openflow/openflow-ext.h"
#include "of_ext_msg.h"
#include "netdev.h"
#include "datapath.h"

/* TODO: Remove when development is over */
#define THIS_MODULE VLM_slicing
#include "vlog.h"

/** Search the datapath for a specific port.
 * @param port_no specified port
 * @param dp the datapath to search
 * @return port if found, NULL otherwise
 */
static struct sw_port *
port_from_port_no(struct datapath *dp,uint16_t port_no) 
{
	struct sw_port * p;
	LIST_FOR_EACH(p, struct sw_port, node, &dp->port_list) {
		if(p->port_no == port_no){
			return p;
		}
	}
	return NULL;
}

/** Search the port for a specific queue.
 * @param queue_id specified queue id
 * @param p the port to query
 * @return queue if found, NULL otherwise
 */
static struct sw_queue *
queue_from_queue_id(struct sw_port *p, uint32_t queue_id)
{
	struct sw_queue *q;

	LIST_FOR_EACH(q, struct sw_queue, node, &p->queue_list) {
		if(q->queue_id == queue_id) {
			return q;
		}
	}
	return NULL;
}

static int
new_queue(struct sw_port * port, struct sw_queue * queue, 
		  uint32_t queue_id, uint16_t class_id, struct ofp_queue_prop_min_rate * mr)
{
	memset(queue, '\0', sizeof *queue);
	queue->port = port;
	queue->queue_id = queue_id;
	/* class_id is the internal mapping to class. It is the offset
	 * in the array of queues for each port. Note that class_id is 
	 * local to port, so we don't have any conflict.
	 * tc uses 16-bit class_id, so we cannot use the queue_id
	 * field */
	queue->class_id = class_id;
	queue->property = ntohs(mr->prop_header.property);
	queue->min_rate = ntohs(mr->rate);

	list_push_back(&port->queue_list, &queue->node);

	return 0;
}

static int
port_add_queue(struct sw_port *p, uint32_t queue_id, struct ofp_queue_prop_min_rate * mr)
{
	int queue_no;
	for (queue_no = 1; queue_no < NETDEV_MAX_QUEUES; queue_no++) {
		struct sw_queue *q = &p->queues[queue_no];
		if(!q->port) {
			return new_queue(p,q,queue_id,queue_no,mr);
		}
	}
	return EXFULL;
}

/** Modifies/adds a queue. It first search if a queue with
 * id exists for this port. If yes it modifies it, otherwise adds
 * a ndw configuration.
 *
 * @param dp the related datapath
 * @param sender request source
 * @param oh the openflow message for queue mod.
 */
static void
recv_of_exp_queue_modify(struct datapath *dp, 
                         const struct sender *sender UNUSED,
                         const void *oh)
{
	struct sw_port *p;
	struct sw_queue *q;
	struct openflow_queue_command_header * ofq_modify;
	struct ofp_packet_queue *opq;
	struct ofp_queue_prop_min_rate *mr;

	int error;
	uint16_t port_no;
	uint32_t queue_id;


	ofq_modify = (struct openflow_queue_command_header *)oh;
	opq = (struct ofp_packet_queue *)ofq_modify->body;
	mr = (struct ofp_queue_prop_min_rate*)opq->properties;

	port_no = ntohs(ofq_modify->port);
	queue_id = ntohl(opq->queue_id);

	p = port_from_port_no(dp, port_no);
	if(p){
		q = queue_from_queue_id(p, queue_id);
		if (q) {
			/* queue exists - modify it */
			error = netdev_change_class(p->netdev,q->class_id, ntohs(mr->rate));
			if (error) {
				VLOG_ERR("Failed to update queue %d", queue_id);
				/* TODO: send appropriate error */
			}
			else {
				/* TODO : Should call tc here to modify the queue */
				q->property = ntohs(mr->prop_header.property);
				q->min_rate = ntohs(mr->rate);
			}
		}
		else {
			/* create new queue */
			port_add_queue(p,queue_id, mr);
			q = queue_from_queue_id(p, queue_id);
			error = netdev_setup_class(p->netdev,q->class_id, ntohs(mr->rate));
			if (error) {
				VLOG_ERR("Failed to configure queue %d", queue_id);
				/* TODO: send appropriate error */
			}
		}
	}
	else {
		VLOG_ERR("Failed to create/modify queue - port %d doesn't exist",port_no);
	}
}

/**
 * Receives an experimental message and pass it
 * to the appropriate handler 
 */
int of_ext_recv_msg(struct datapath *dp, const struct sender *sender,
        const void *oh)
{
    const struct openflow_queue_command_header  *ofexth = oh;

    switch (ntohl(ofexth->header.subtype)) {
    case OFP_EXT_QUEUE_MODIFY: {
		recv_of_exp_queue_modify(dp,sender,oh);
		return 0;
    }
	case OFP_EXT_QUEUE_DELETE: {
		return 0;
    }
    default:
		VLOG_ERR("Received unknown command of type %d",
                 ntohl(ofexth->header.subtype));
		return -EINVAL;
    }

    return -EINVAL;
}
