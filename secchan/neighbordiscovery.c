#include "neighbordiscovery.h"
#include "ofpbuf.h"
#include "xtoxll.h"

#define THIS_MODULE VLM_neighbor_discovery
#include "vlog.h"

/** \brief Callback for periodic checking.
 *
 * The function does the following:
 * <ul>
 * <li>Check for neighbor expired</li>
 * <li>Check for need to probe and send them</li>
 * <\ul>
 *
 * @param nd_ pointer to state for neighbor discovery
 */
static void neighbordiscovery_periodic_cb(void *nd_)
{
  uint16_t portno;
  struct neighbor_discovery* nd = nd_;
  struct timeval now, tincrement, tresult;
  struct ofpbuf* msg;

  gettimeofday(&now, NULL);
  VLOG_DBG("Periodic call at %ld.%.6ld", now.tv_sec, now.tv_usec);

  //Do not run if no controller is connected
  if (!rconn_is_connected(nd->remote_rconn))
  {
    VLOG_DBG("Not activated <= no remote controller!");
    return;
  }

  //Check for expired neighbor
  

  //Send probe for port
  if (nd->probe_ready && rconn_is_connected(nd->local_rconn))
    for (portno=1; portno <= nd->max_portno; portno++)
      if ((nd->port[portno].expiryTime.tv_sec != 0) && 
	  (nd->port[portno].expiryTime.tv_usec != 0) &&
	  (timercmp(&now, &nd->port[portno].expiryTime, >=)))
      {
	//Send probe
	nd->probe.oao.port = htons(portno);
	nd->probe.outport = htons(portno);
	nd->probe.interval = nd->min_miss_probe * \
	  (nd->port[portno].neighbor_no == 0)?nd->idle_probe_interval: \
	  nd->active_probe_interval;
	
	msg = ofpbuf_new(sizeof(nd->probe));
	ofpbuf_put(msg, &(nd->probe), sizeof(nd->probe));
	rconn_send(nd->local_rconn, msg, NULL);
	VLOG_DBG("Send LLDP probe to port %u", portno);

	//Update probe table
	if (nd->port[portno].neighbor_no == 0)
	  tincrement.tv_sec = nd->idle_probe_interval;
	else
	  tincrement.tv_sec = nd->active_probe_interval;
	tincrement.tv_usec = 0;
	timeradd(&now, &tincrement, &tresult);
	nd->port[portno].expiryTime = tresult;
      }

  VLOG_DBG("End of periodic!");
}

/** \brief allback for local packet
 *
 * The function does the following:
 * <ul>
 * <li>Get feature reply to parse datapath id of switch</li>
 * <li>Get feature reply to identify valid ports</li>
 * <\ul>
 *
 * @param r reference to relay
 * @param nd_ pointer to state for neighbor discovery
 */
static bool neighbordiscovery_local_packet_cb(struct relay *r, void *nd_)
{
  int i, j;
  uint16_t portno;
  struct timeval now, tincrement, tresult;
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
    nd->probe.datapath_id = osf->datapath_id;
    VLOG_DBG("Received features from switch with dpid %llx",
	     ntohll(osf->datapath_id)); 

    i = (ntohs(oh->length)-sizeof(*osf))/sizeof(*opp);
    opp = osf->ports;
    nd->max_portno = 0;
    tincrement.tv_sec = nd->idle_probe_interval;
    tincrement.tv_usec = 0;
    timeradd(&now, &tincrement, &tresult);
    //Iterate each port
    for (j = 0; j < i ; j++)
    {
      if (ntohs(opp->port_no) <= OFPP_MAX)
      {
	//Register maximum port no
	portno = ntohs(opp->port_no);
	if (nd->max_portno < portno)
	  nd->max_portno = portno;
	//Register port
	nd->port[portno].expiryTime = tresult;
	VLOG_DBG("Port %u registered", portno);
      }
      opp++;
    }
    nd->probe_ready = true;
  }

  //Update neighbor if LLDP packet is received
  

  return false;
}

/** Hook class to declare callback functions.
 */
static struct hook_class neighbordiscovery_hook_class =
{
  neighbordiscovery_local_packet_cb, /* local_packet_cb */
  NULL,                              /* remote_packet_cb */
  neighbordiscovery_periodic_cb,     /* periodic_cb */
  NULL,                              /* wait_cb */
  NULL,                              /* closing_cb */
};

void neighbordiscovery_start(struct secchan *secchan,
			     struct rconn *local_rconn, 
			     struct rconn *remote_rconn,
			     struct neighbor_discovery** nd_ptr)
{
  int i;
  struct neighbor_discovery* nd;

  //Allocate memory for state
  nd = *nd_ptr = xcalloc(1, sizeof *nd);

  //Settings for neighbor discovery
  nd->local_rconn = local_rconn;
  nd->remote_rconn = remote_rconn;
  nd->probe_ready = false;
  nd->idle_probe_interval = NEIGHBOR_DEFAULT_IDLE_INTERVAL;
  nd->active_probe_interval = NEIGHBOR_DEFAULT_ACTIVE_INTERVAL;
  nd->min_miss_probe = NEIGHBOR_MIN_MISS_PROBE;

  //Prepack neighbor packet
  nd->neighbormsg.header.version = OFP_VERSION;
  nd->neighbormsg.header.type = OFPT_NEIGHBOR_MSG;
  nd->neighbormsg.header.length = htons(sizeof(struct ofp_neighbor_msg));
  nd->neighbormsg.header.xid = htons(0);

  //Prepack probe packet
  nd->probe.pktout.header.version = OFP_VERSION;
  nd->probe.pktout.header.type = OFPT_PACKET_OUT;
  nd->probe.pktout.header.length = htons(sizeof(nd->probe));
  nd->probe.pktout.header.xid = htonl(0);
  nd->probe.pktout.buffer_id = htonl(-1);
  nd->probe.pktout.in_port=htons(OFPP_NONE);
  nd->probe.pktout.actions_len = htons(1);
  nd->probe.oao.type= htons(OFPAT_OUTPUT);
  nd->probe.oao.len = htons(8);
  nd->probe.oao.max_len=0; 
                           //Use Stanford OUI with zero thereafter
  nd->probe.ethhdr.ether_shost[0]=0x08;
  nd->probe.ethhdr.ether_shost[1]=0x00;
  nd->probe.ethhdr.ether_shost[2]=0x56;
  nd->probe.ethhdr.ether_shost[3]=0x00;
  nd->probe.ethhdr.ether_shost[4]=0x00;
  nd->probe.ethhdr.ether_shost[5]=0x00;
                           //Use IEEE 802.1AB LLDP multicast ethernet address
  nd->probe.ethhdr.ether_dhost[0]=0x01;
  nd->probe.ethhdr.ether_dhost[1]=0x80;
  nd->probe.ethhdr.ether_dhost[2]=0xC2;
  nd->probe.ethhdr.ether_dhost[3]=0x00;
  nd->probe.ethhdr.ether_dhost[4]=0x00;
  nd->probe.ethhdr.ether_dhost[5]=0x0E;
                           //Use IEEE 802.1AB ethertype
  nd->probe.ethhdr.ether_type = htons(OPENFLOW_LLDP_TYPE);
  
  //Use invalid port OFPP_NONE to denote empty entry
  for (i = 0; i < NEIGHBOR_PORT_MAX_NO; i++)
  {
    nd->port[i].neighbor_no = 0;
    nd->port[i].expiryTime.tv_sec = 0;
    nd->port[i].expiryTime.tv_usec = 0;
  }

  //Denote empty port with expiry time of 0
  for (i = 0; i < NEIGHBOR_MAX_NO; i++)
    nd->neighbors[i].in_port = OFPP_NONE;
  
  //Register hooks
  add_hook(secchan, &neighbordiscovery_hook_class, nd);
}

