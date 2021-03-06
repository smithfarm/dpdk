/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2009-2018 Microsoft Corp.
 * Copyright (c) 2016 Brocade Communications Systems, Inc.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2012 Citrix Inc.
 * All rights reserved.
 */

/*
 * Tunable ethdev params
 */
#define HN_MIN_RX_BUF_SIZE	1024
#define HN_MAX_XFER_LEN		2048
#define	HN_MAX_MAC_ADDRS	1
#define HN_MAX_CHANNELS		64

/* Claimed to be 12232B */
#define HN_MTU_MAX		(9 * 1024)

/* Retry interval */
#define HN_CHAN_INTERVAL_US	100

/* Buffers need to be aligned */
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef PAGE_MASK
#define PAGE_MASK (PAGE_SIZE - 1)
#endif

struct hn_data;
struct hn_txdesc;

struct hn_stats {
	uint64_t	packets;
	uint64_t	bytes;
	uint64_t	errors;
	uint64_t	nomemory;
	uint64_t	multicast;
	uint64_t	broadcast;
	/* Size bins in array as RFC 2819, undersized [0], 64 [1], etc */
	uint64_t	size_bins[8];
};

struct hn_tx_queue {
	struct hn_data  *hv;
	struct vmbus_channel *chan;
	uint16_t	port_id;
	uint16_t	queue_id;
	uint32_t	free_thresh;

	/* Applied packet transmission aggregation limits. */
	uint32_t	agg_szmax;
	uint32_t	agg_pktmax;
	uint32_t	agg_align;

	/* Packet transmission aggregation states */
	struct hn_txdesc *agg_txd;
	uint32_t	agg_pktleft;
	uint32_t	agg_szleft;
	struct rndis_packet_msg *agg_prevpkt;

	struct hn_stats stats;
};

struct hn_rx_queue {
	struct hn_data  *hv;
	struct vmbus_channel *chan;
	struct rte_mempool *mb_pool;
	struct rte_ring *rx_ring;
	void	*event_buf;

	rte_spinlock_t ring_lock;
	uint32_t event_sz;
	uint16_t port_id;
	uint16_t queue_id;
	struct hn_stats stats;
	uint64_t ring_full;
};


/* multi-packet data from host */
struct hn_rx_bufinfo {
	struct vmbus_channel *chan;
	struct hn_data *hv;
	uint64_t	xactid;
	struct rte_mbuf_ext_shared_info shinfo;
} __rte_cache_aligned;

struct hn_data {
	struct rte_vmbus_device *vmbus;
	struct hn_rx_queue *primary;
	uint16_t	port_id;
	bool		closed;
	uint32_t	link_status;
	uint32_t	link_speed;

	struct rte_mem_resource *rxbuf_res;	/* UIO resource for Rx */
	struct hn_rx_bufinfo *rxbuf_info;
	uint32_t	rxbuf_section_cnt;	/* # of Rx sections */
	volatile uint32_t rxbuf_outstanding;
	uint16_t	max_queues;		/* Max available queues */
	uint16_t	num_queues;
	uint64_t	rss_offloads;

	struct rte_mem_resource *chim_res;	/* UIO resource for Tx */
	struct rte_mempool *tx_pool;		/* Tx descriptors */
	uint32_t	chim_szmax;		/* Max size per buffer */
	uint32_t	chim_cnt;		/* Max packets per buffer */

	uint32_t	nvs_ver;
	uint32_t	ndis_ver;
	uint32_t	rndis_agg_size;
	uint32_t	rndis_agg_pkts;
	uint32_t	rndis_agg_align;

	volatile uint32_t  rndis_pending;
	rte_atomic32_t	rndis_req_id;
	uint8_t		rndis_resp[256];

	struct ether_addr mac_addr;
	struct vmbus_channel *channels[HN_MAX_CHANNELS];
};

static inline struct vmbus_channel *
hn_primary_chan(const struct hn_data *hv)
{
	return hv->channels[0];
}

void hn_process_events(struct hn_data *hv, uint16_t queue_id);

uint16_t hn_xmit_pkts(void *tx_queue, struct rte_mbuf **tx_pkts,
		      uint16_t nb_pkts);
uint16_t hn_recv_pkts(void *rx_queue, struct rte_mbuf **rx_pkts,
		      uint16_t nb_pkts);

int	hn_tx_pool_init(struct rte_eth_dev *dev);
int	hn_dev_tx_queue_setup(struct rte_eth_dev *dev, uint16_t queue_idx,
			      uint16_t nb_desc, unsigned int socket_id,
			      const struct rte_eth_txconf *tx_conf);
void	hn_dev_tx_queue_release(void *arg);

struct hn_rx_queue *hn_rx_queue_alloc(struct hn_data *hv,
				      uint16_t queue_id,
				      unsigned int socket_id);
int	hn_dev_rx_queue_setup(struct rte_eth_dev *dev,
			      uint16_t queue_idx, uint16_t nb_desc,
			      unsigned int socket_id,
			      const struct rte_eth_rxconf *rx_conf,
			      struct rte_mempool *mp);
void	hn_dev_rx_queue_release(void *arg);
