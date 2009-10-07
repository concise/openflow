/* Copyright (c) 2008, 2009 The Board of Trustees of The Leland Stanford
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

/** \page neighbor_discovery_page Neighbor Discovery
 * This feature allows the OpenFlow switch to discover neighboring
 * OpenFlow switches.  This is done using a custom LLDP probe packet.
 * 
 * @date October 2009
 * @author ykk
 *
 * Warning: Though reading the 5377+ lines of C is left as an exercise
 * for all coders that help to add features into secchan, I have not 
 * got through the exercise when I coded this feature.  Do not imitate
 * how this feature is implemented.  
 */

#ifndef NEIGHBOR_DISCOVERY_H
#define NEIGHBOR_DISCOVERY_H 1

#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include "secchan.h"
#include "rconn.h"

/** Maximum number of neighbors tracked.
 */
#define NEIGHBOR_MAX_NO 255

/** Maximum number of ports tracked.
 */
#define NEIGHBOR_PORT_MAX_NO 255

/** Default interval to send on idle port (in seconds)
 */
#define NEIGHBOR_DEFAULT_IDLE_INTERVAL 8

/** Default interval to send on active port (in seconds)
 */
#define NEIGHBOR_DEFAULT_ACTIVE_INTERVAL 1

/** Default maximum interval of silence before
 * neighbor is perceived as lost (in seconds)
 */
#define NEIGHBOR_MAX_MISS_INTERVAL 10

/** Structure to contain information about a neighbor
 */
struct neighbor
{
  /** Port neighbor is connected to.
   */
  uint16_t in_port;
  /** Datapath id of neighbor.
   */
  uint64_t neighbor_dpid;
  /** Port neighbor is connected from.
   */
  uint16_t neighbor_port;
  /** Expiry time.
   */
  struct timeval expiryTime;
};

/** \brief Probe packet.
 * This is an Ethernet packet with custom payload.
 */
struct neighbor_probe
{
  /** Out port probe packet is sent (cannot be prepacked).
   */
  uint16_t outport;
  /** Datapath id of this switch.
   */
  uint64_t datapath_id;
  /** Next delivery time.
   */
  uint8_t interval;
};

/** State for neighbor discovery
 */
struct neighbor_discovery
{
  /** Connection to local switch
   */
  struct rconn* local_rconn;
  /** Connection to remote controller
   */
  struct rconn* remote_rconn;
  /** Interval between messages for idle port.
   */
  uint8_t idle_probe_interval;
  /** Interval between messages for connected port.
   */
  uint8_t active_probe_interval;
  /** Minimum interval of silence before declaring neighbor is disconnected.
   * Should be greater than active_probe_interval.
   */
  uint8_t max_miss_interval;
  /** Local rconn
   */
  /** Prepacked probe packet.
   */
  struct neighbor_probe probe;
  /** List of neighbors
   */
  struct neighbor neighbors[NEIGHBOR_MAX_NO];
  /** List of neighbors
   */
  struct timeval portTime[NEIGHBOR_PORT_MAX_NO];
};

/** Initialize neighbor discovery
 * @param secchan secchan for adding hooks to
 * @param nd_ptr pointer to state for neighbor discovery to populate
 */
void neighbordiscovery_start(struct secchan *secchan,
			     struct rconn *local_rconn, 
			     struct rconn *remote_rconn,
			     struct neighbor_discovery** nd_ptr);

/** Periodic checking of port state
 * @param nd pointer to state for neighbor discovery
 */
static void neighbordiscovery_periodic_cb(void *nd);

/** Callback for local packet
 * @param r reference to relay
 * @param nd pointer to state for neighbor discovery
 */
static bool neighbordiscovery_local_packet_cb(struct relay *r, void *nd);

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

#endif
