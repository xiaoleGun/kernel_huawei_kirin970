#ifndef __NET_FRAG_H__
#define __NET_FRAG_H__

struct netns_frags {
	/* Keep atomic mem on separate cachelines in structs that include it */
	atomic_t		mem ____cacheline_aligned_in_smp;
	/* sysctls */
	int			timeout;
	int			high_thresh;
	int			low_thresh;
	int			max_dist;
};

/**
 * fragment queue flags
 *
 * @INET_FRAG_FIRST_IN: first fragment has arrived
 * @INET_FRAG_LAST_IN: final fragment has arrived
 * @INET_FRAG_COMPLETE: frag queue has been processed and is due for destruction
 */
enum {
	INET_FRAG_FIRST_IN	= BIT(0),
	INET_FRAG_LAST_IN	= BIT(1),
	INET_FRAG_COMPLETE	= BIT(2),
};

/**
 * struct inet_frag_queue - fragment queue
 *
 * @lock: spinlock protecting the queue
 * @timer: queue expiration timer
 * @list: hash bucket list
 * @refcnt: reference count of the queue
 * @fragments: received fragments head
 * @fragments_tail: received fragments tail
 * @stamp: timestamp of the last received fragment
 * @len: total length of the original datagram
 * @meat: length of received fragments so far
 * @flags: fragment queue flags
 * @max_size: maximum received fragment size
 * @net: namespace that this frag belongs to
 * @list_evictor: list of queues to forcefully evict (e.g. due to low memory)
 */
struct inet_frag_queue {
	spinlock_t		lock;
	struct timer_list	timer;
	struct hlist_node	list;
	atomic_t		refcnt;
	struct sk_buff		*fragments;
	struct sk_buff		*fragments_tail;
	ktime_t			stamp;
	int			len;
	int			meat;
	__u8			flags;
	u16			max_size;
	struct netns_frags	*net;
	struct hlist_node	list_evictor;
};

#define INETFRAGS_HASHSZ	1024

/* averaged:
 * max_depth = default ipfrag_high_thresh / INETFRAGS_HASHSZ /
 *	       rounded up (SKB_TRUELEN(0) + sizeof(struct ipq or
 *	       struct frag_queue))
 */
#define INETFRAGS_MAXDEPTH	128

struct inet_frag_bucket {
	struct hlist_head	chain;
	spinlock_t		chain_lock;
};

struct inet_frags {
	struct inet_frag_bucket	hash[INETFRAGS_HASHSZ];

	struct work_struct	frags_work;
	unsigned int next_bucket;
	unsigned long last_rebuild_jiffies;
	bool rebuild;

	/* The first call to hashfn is responsible to initialize
	 * rnd. This is best done with net_get_random_once.
	 *
	 * rnd_seqlock is used to let hash insertion detect
	 * when it needs to re-lookup the hash chain to use.
	 */
	u32			rnd;
	seqlock_t		rnd_seqlock;
	int			qsize;

	unsigned int		(*hashfn)(const struct inet_frag_queue *);
	bool			(*match)(const struct inet_frag_queue *q,
					 const void *arg);
	void			(*constructor)(struct inet_frag_queue *q,
					       const void *arg);
	void			(*destructor)(struct inet_frag_queue *);
	void			(*frag_expire)(unsigned long data);
	struct kmem_cache	*frags_cachep;
	const char		*frags_cache_name;
};

int inet_frags_init(struct inet_frags *);
void inet_frags_fini(struct inet_frags *);

static inline void inet_frags_init_net(struct netns_frags *nf)
{
	atomic_set(&nf->mem, 0);
}
void inet_frags_exit_net(struct netns_frags *nf, struct inet_frags *f);

void inet_frag_kill(struct inet_frag_queue *q, struct inet_frags *f);
void inet_frag_destroy(struct inet_frag_queue *q, struct inet_frags *f);
struct inet_frag_queue *inet_frag_find(struct netns_frags *nf,
		struct inet_frags *f, void *key, unsigned int hash);

void inet_frag_maybe_warn_overflow(struct inet_frag_queue *q,
				   const char *prefix);

static inline void inet_frag_put(struct inet_frag_queue *q, struct inet_frags *f)
{
	if (atomic_dec_and_test(&q->refcnt))
		inet_frag_destroy(q, f);
}

static inline bool inet_frag_evicting(struct inet_frag_queue *q)
{
	return !hlist_unhashed(&q->list_evictor);
}

/* Memory Tracking Functions. */

static inline int frag_mem_limit(struct netns_frags *nf)
{
	return atomic_read(&nf->mem);
}

static inline void sub_frag_mem_limit(struct netns_frags *nf, int i)
{
	atomic_sub(i, &nf->mem);
}

static inline void add_frag_mem_limit(struct netns_frags *nf, int i)
{
	atomic_add(i, &nf->mem);
}

static inline int sum_frag_mem_limit(struct netns_frags *nf)
{
	return atomic_read(&nf->mem);
}

/* RFC 3168 support :
 * We want to check ECN values of all fragments, do detect invalid combinations.
 * In ipq->ecn, we store the OR value of each ip4_frag_ecn() fragment value.
 */
#define	IPFRAG_ECN_NOT_ECT	0x01 /* one frag had ECN_NOT_ECT */
#define	IPFRAG_ECN_ECT_1	0x02 /* one frag had ECN_ECT_1 */
#define	IPFRAG_ECN_ECT_0	0x04 /* one frag had ECN_ECT_0 */
#define	IPFRAG_ECN_CE		0x08 /* one frag had ECN_CE */

extern const u8 ip_frag_ecn_table[16];

/* Return values of inet_frag_queue_insert() */
#define IPFRAG_OK	0
#define IPFRAG_DUP	1
#define IPFRAG_OVERLAP	2
int inet_frag_queue_insert(struct inet_frag_queue *q, struct sk_buff *skb,
			   int offset, int end);
void *inet_frag_reasm_prepare(struct inet_frag_queue *q, struct sk_buff *skb,
			      struct sk_buff *parent);
void inet_frag_reasm_finish(struct inet_frag_queue *q, struct sk_buff *head,
			    void *reasm_data);
struct sk_buff *inet_frag_pull_head(struct inet_frag_queue *q);

#endif
