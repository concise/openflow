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

/* TODO: Remove when development is over */
#define THIS_MODULE VLM_slicing
#include "vlog.h"

int of_ext_recv_msg(struct datapath *dp UNUSED, const struct sender *sender UNUSED,
        const void *oh)
{
    const struct openflow_queue_command_header  *ofexth = oh;

    switch (ntohl(ofexth->subtype)) {
    case OFQ_CMD_MODIFY: {
		VLOG_ERR("Received OFQ_CMD_MODIFY command");
		return 0;
    }
	case OFQ_CMD_DELETE: {
		VLOG_ERR("Received OFQ_CMD_DELETE command");
		return 0;
    }
	case OFQ_CMD_SHOW: {
		VLOG_ERR("Received OFQ_CMD_SHOW command");
		return 0;
    }
    default:
		VLOG_ERR("Received unknown command of type %d",ntohl(ofexth->subtype));
		return -EINVAL;
    }

    return -EINVAL;
}
