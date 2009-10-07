#include "neighbordiscovery.h"
#include "openflow/openflow.h"
#include "ofpbuf.h"
#include "xtoxll.h"

#define THIS_MODULE VLM_neighbor_discovery
#include "vlog.h"

static void neighbordiscovery_periodic_cb(void *nd_)
{
  struct neighbor_discovery* nd = nd_;
  struct timeval now;
  gettimeofday(&now, NULL);
  fprintf(stderr, "%ld.%.6ld\n", now.tv_sec, now.tv_usec);

  //Do not run if no controller is connected
  if (!rconn_is_connected(nd->remote_rconn))
  {
    VLOG_DBG("Neighbor discovery not activated: no controller!");
    return;
  }

  //Check for expired neighbor

  //Send probe for ports

}

static bool neighbordiscovery_local_packet_cb(struct relay *r, void *nd_)
{
  struct timeval now;
  struct neighbor_discovery* nd = nd_;
  struct ofpbuf *msg = r->halves[HALF_LOCAL].rxbuf;
  struct ofp_header *oh = msg->data;

  gettimeofday(&now, NULL);
  fprintf(stderr, "%ld.%.6ld\n", now.tv_sec, now.tv_usec);

  //Update neighbor if LLDP packet received
  

  return false;
}

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
  nd->idle_probe_interval = NEIGHBOR_DEFAULT_IDLE_INTERVAL;
  nd->active_probe_interval = NEIGHBOR_DEFAULT_ACTIVE_INTERVAL;
  nd->max_miss_interval = NEIGHBOR_MAX_MISS_INTERVAL;

  //Prepack probe packet

  //in port 0 is invalid, and thus used to denote empty entry
  for (i = 0; i < NEIGHBOR_MAX_NO; i++)
    nd->neighbors[i].in_port = 0;

  //Register hooks
  add_hook(secchan, &neighbordiscovery_hook_class, nd);
}

