#include "neighbordiscovery.h"
#include "openflow/openflow.h"
#include "ofpbuf.h"
#include "xtoxll.h"

#define THIS_MODULE VLM_neighbor_discovery
#include "vlog.h"

/** Periodic checking of port state
 * @param nd_ pointer to state for neighbor discovery
 */
static void neighbordiscovery_periodic_cb(void *nd_)
{
  struct neighbor_discovery* nd = nd_;
  struct timeval now;
  gettimeofday(&now, NULL);
  fprintf(stderr, "periodic: %ld.%.6ld\n", now.tv_sec, now.tv_usec);

  //Do not run if no controller is connected
  if (!rconn_is_connected(nd->remote_rconn))
  {
    VLOG_DBG("Not activated <= no remote controller!");
    return;
  }

  //Check for expired neighbor
  

  //Send probe for port

  VLOG_DBG("End of periodic!\n");
}

/** Callback for local packet
 * @param r reference to relay
 * @param nd_ pointer to state for neighbor discovery
 */
static bool neighbordiscovery_local_packet_cb(struct relay *r, void *nd_)
{
  int i, j;
  uint16_t portno;
  struct timeval now;
  struct neighbor_discovery* nd = nd_;
  struct ofpbuf *msg = r->halves[HALF_LOCAL].rxbuf;
  struct ofp_header *oh = msg->data;
  struct ofp_switch_features *osf;
  struct ofp_phy_port *opp;

  gettimeofday(&now, NULL);
  fprintf(stderr, "packet: %ld.%.6ld\n", now.tv_sec, now.tv_usec);

  //Register features reply
  if (oh->type == OFPT_FEATURES_REPLY)
  {
    osf = (struct ofp_switch_features *) oh;
    nd->payload->datapath_id = osf->datapath_id;
    VLOG_DBG("Received features from switch with dpid %llx",
	     ntohll(osf->datapath_id)); 

    i = (ntohs(oh->length)-sizeof(*osf))/sizeof(*opp);
    opp = osf->ports;
    nd->max_portno = 0;
    //Iterate each port
    for (j = 0; j < i ; j++)
    {
      if (ntohs(opp->port_no) <= OFPP_MAX)
      {
	//Register maximum port no
	portno = ntohs(opp->port_no);
	if (nd->max_portno < portno)
	  nd->max_portno = portno;
	VLOG_DBG("Port %u registered", portno);
      }
      opp++;
    }
  }

  //Update neighbor if LLDP packet is received
  

  return false;
}

/** Callback for closing.
 * Delete buffer.
 * @param r reference to relay
 * @param nd_ pointer to state for neighbor discovery
 */
static void neighbordiscovery_closing_cb(struct relay *r, void *nd_)
{
  struct neighbor_discovery* nd = nd_;
  ofpbuf_delete(nd->probe);
  ofpbuf_delete(nd->neighbormsg);
}

/** Hook class to declare callback functions.
 */
static struct hook_class neighbordiscovery_hook_class =
{
  neighbordiscovery_local_packet_cb, /* local_packet_cb */
  NULL,                              /* remote_packet_cb */
  neighbordiscovery_periodic_cb,     /* periodic_cb */
  NULL,                              /* wait_cb */
  neighbordiscovery_closing_cb,      /* closing_cb */
};

void neighbordiscovery_start(struct secchan *secchan,
			     struct rconn *local_rconn, 
			     struct rconn *remote_rconn,
			     struct neighbor_discovery** nd_ptr)
{
  int i;
  struct neighbor_discovery* nd;
  size_t pktsize;
  struct ofp_packet_out* pktout;
  struct ether_header* ethhdr;

  //Allocate memory for state
  nd = *nd_ptr = xcalloc(1, sizeof *nd);

  //Settings for neighbor discovery
  nd->local_rconn = local_rconn;
  nd->remote_rconn = remote_rconn;
  nd->probe_ready = false;
  nd->idle_probe_interval = NEIGHBOR_DEFAULT_IDLE_INTERVAL;
  nd->active_probe_interval = NEIGHBOR_DEFAULT_ACTIVE_INTERVAL;
  nd->max_miss_interval = NEIGHBOR_MAX_MISS_INTERVAL;

  //Prepack neighbor packet
  pktsize = sizeof(struct ofp_neighbor_msg);
  nd->neighbormsg = ofpbuf_new(pktsize);
  nd->onm = nd->neighbormsg->data;
  nd->onm->header.version = OFP_VERSION;
  nd->onm->header.type = OFPT_NEIGHBOR_MSG;
  nd->onm->header.length = htons(pktsize);
  nd->onm->header.xid = htons(0);

  //Prepack probe packet
  pktsize = sizeof(struct ofp_packet_out)+
    sizeof(struct ether_header)+
    sizeof(struct neighbor_probe_payload);
  nd->probe = ofpbuf_new(pktsize);
  pktout = nd->probe->data;
  pktout->header.version = OFP_VERSION;
  pktout->header.type = OFPT_PACKET_OUT;
  pktout->header.length = htons(pktsize);
  pktout->header.xid = htonl(0);
  pktout->buffer_id = htonl(-1);
  pktout->in_port=htons(OFPP_NONE);
  pktout->actions_len = htons(1);
  nd->oao = (struct ofp_action_output*) pktout->actions;
  nd->oao->type= htons(OFPAT_OUTPUT);
  nd->oao->len = htons(8);
  nd->oao->max_len=0; 
  ethhdr = (struct ether_header*) (nd->oao+1);
                           //Use Stanford OUI with zero thereafter
  ethhdr->ether_shost[0]=0x08;
  ethhdr->ether_shost[1]=0x00;
  ethhdr->ether_shost[2]=0x56;
  ethhdr->ether_shost[3]=0x00;
  ethhdr->ether_shost[4]=0x00;
  ethhdr->ether_shost[5]=0x00;
                           //Use IEEE 802.1AB LLDP multicast ethernet address
  ethhdr->ether_dhost[0]=0x01;
  ethhdr->ether_dhost[1]=0x80;
  ethhdr->ether_dhost[2]=0xC2;
  ethhdr->ether_dhost[3]=0x00;
  ethhdr->ether_dhost[4]=0x00;
  ethhdr->ether_dhost[5]=0x0E;
                           //Use IEEE 802.1AB ethertype
  ethhdr->ether_type = htons(OPENFLOW_LLDP_TYPE);
  nd->payload= (struct neighbor_probe_payload*) (ethhdr+1);

  //Use invalid port OFPP_NONE to denote empty entry
  for (i = 0; i < NEIGHBOR_MAX_NO; i++)
    nd->neighbors[i].in_port = OFPP_NONE;
  

  //Register hooks
  add_hook(secchan, &neighbordiscovery_hook_class, nd);
}

