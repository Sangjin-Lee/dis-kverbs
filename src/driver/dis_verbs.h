#ifndef __DIS_VERBS_H__
#define __DIS_VERBS_H__

#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/wait.h>

#include <rdma/ib_verbs.h>
#include <rdma/ib_mad.h>

#include "scilib.h"

#define DIS_WQE_SGE_MAX 4
#define DIS_QP_INI      100
#define DIS_QP_MAX      200
#define DIS_MR_INI      0
#define DIS_MR_MAX      100
#define DIS_PB_PER_MAP  (PAGE_SIZE / sizeof(struct dis_pb))

enum dis_wq_flag {
    DIS_WQ_EMPTY,
    DIS_WQ_POST,
};

enum dis_wq_status {
    DIS_WQ_UNINITIALIZED,
    DIS_WQ_INITIALIZED,
    DIS_WQ_RUNNING,
    DIS_WQ_EXITED,
};

enum dis_wq_type {
    DIS_SQ,
    DIS_RQ,
};

struct dis_pb {
    u64 phys_addr;
    u64 length;
};

struct dis_map {
    struct dis_pb pb_list[DIS_PB_PER_MAP];
};

struct dis_dev {
    struct ib_device                ibdev;
    struct device                   *dev;
    struct device_dma_parameters    dma_parms;
};

struct dis_mr {
    struct ib_mr    ibmr;
    struct ib_umem  *ibumem;
    // struct ib_pd    *ibpd;
    dma_addr_t      dma_addr;
    struct dis_map  **map_list;
    // struct dis_pbl  pbl_table;
    u64             virt_addr;
    u32             map_count;
    u32             page_count;
    u32             offset;
};

struct dis_ah {
    struct ib_ah    ibah;
    struct dis_dev  *dev;
};

struct dis_cqe {
    struct ib_qp    *ibqp;
    u32             id;
    u16             status;
    u16             byte_len;
    u8              valid;
    u8              opcode;
};

struct dis_cq {
    struct ib_cq    ibcq;
    struct dis_dev  *dev;
    struct dis_cqe  *cqe_queue;
    spinlock_t      cqe_lock;
    u32             cqe_get;
    u32             cqe_put;
    u32             cqe_max;
};

struct dis_wqe {
    struct ib_qp    *ibqp;
    struct iovec    iov[DIS_WQE_SGE_MAX];
    sci_msq_queue_t *sci_msq;
    sci_msg_t       sci_msg;
    u32             lkey[DIS_WQE_SGE_MAX];
    u32             id;
    u32             l_qpn;
    u32             byte_len;
    u8              opcode;
    u8              valid;
};

struct dis_wq {
    struct dis_cq       *cq;
    struct dis_wqe      *wqe_queue;
    struct task_struct  *thread;
    wait_queue_head_t   wait_queue;
    sci_msq_queue_t     sci_msq;
    enum dis_wq_flag    wq_flag;
    enum dis_wq_status  wq_state;
    enum dis_wq_type    wq_type;
    u32                 wqe_get;
    u32                 wqe_put;
    u32                 wqe_max;
    u32                 sge_max;
    u32                 inline_max;
    u32                 mtu;
    u32                 l_qpn; 
    u32                 r_qpn;
};

struct dis_qp {
    struct ib_qp        ibqp;
    struct dis_dev      *dev;
    struct dis_wq       sq;
    struct dis_wq       rq;
    enum ib_sig_type    sq_sig_type;
    enum ib_qp_type     type;
    enum ib_qp_state    state;
    u32                 l_qpn;
    u32                 r_qpn;
    u32                 mtu;
    void (*event_handler)(struct ib_event *, void *);
};

struct dis_srq {
    struct ib_srq       ibsrq;
    struct dis_wq       rq;
    spinlock_t          srq_lock;
    enum ib_srq_type    srq_type;
    u32                 srq_limit;
    sci_msq_queue_t     sci_msq;
};

struct dis_pd {
    struct ib_pd    ibpd;
    struct dis_dev  *dev;
    struct dis_qp   qp_list[DIS_QP_MAX];
    struct dis_mr   mr_list[DIS_MR_MAX];
    u32             qp_c;
    u32             mr_c;
};


struct dis_ucontext {
    struct ib_ucontext  ibucontext;
    struct dis_dev      *dev;
};

// Custom structure conversion inline functions.
static inline struct dis_dev *to_dis_dev(struct ib_device *ibdev)
{
    return ibdev ? container_of(ibdev, struct dis_dev, ibdev) : NULL;
}

static inline struct dis_pd *to_dis_pd(struct ib_pd *ibpd)
{
    return ibpd ? container_of(ibpd, struct dis_pd, ibpd) : NULL;
}

static inline struct dis_ah *to_dis_ah(struct ib_ah *ibah)
{
    return ibah ? container_of(ibah, struct dis_ah, ibah) : NULL;
}

static inline struct dis_qp *to_dis_qp(struct ib_qp *ibqp)
{
    return ibqp ? container_of(ibqp, struct dis_qp, ibqp) : NULL;
}

static inline struct dis_cq *to_dis_cq(struct ib_cq *ibcq)
{
    return ibcq ? container_of(ibcq, struct dis_cq, ibcq) : NULL;
}

static inline struct dis_srq *to_dis_srq(struct ib_srq *ibsrq)
{
    return ibsrq ? container_of(ibsrq, struct dis_srq, ibsrq) : NULL;
}

static inline struct dis_mr *to_dis_mr(struct ib_mr *ibmr)
{
    return ibmr ? container_of(ibmr, struct dis_mr, ibmr) : NULL;
}

static inline struct dis_ucontext *to_dis_ucontext(struct ib_ucontext *ibucontext)
{
    return ibucontext ? container_of(ibucontext, struct dis_ucontext, ibucontext) : NULL;
}

// Ucontext verbs.
int dis_alloc_ucontext(struct ib_ucontext *ibucontext,
                                struct ib_udata *udata);
void dis_dealloc_ucontext(struct ib_ucontext *ibucontext);

// Device verbs.
int dis_query_device(struct ib_device *ibdev, 
                        struct ib_device_attr *dev_attr,
                        struct ib_udata *udata);
int dis_query_port(struct ib_device *ibdev, 
                    u8 port,
                    struct ib_port_attr *port_attr);
int dis_get_port_immutable(struct ib_device *ibdev, 
                            u8 port_num,
                            struct ib_port_immutable *immutable);
int dis_query_pkey(struct ib_device *ibdev, u8 port, u16 index, u16 *pkey);

// Protection Domain verbs.
int dis_alloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);
void dis_dealloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);

// Memory Region verbs.
struct ib_mr *dis_reg_user_mr(struct ib_pd *ibpd, 
                                u64 start,
                                u64 length,
                                u64 hca_va,
                                int access,
                                struct ib_udata *udata);
struct ib_mr *dis_get_dma_mr(struct ib_pd *ibpd, int access);
int dis_dereg_mr(struct ib_mr *ibmr, struct ib_udata *udata);

// Completion Queue verbs.
int dis_create_cq(struct ib_cq *ibcq,
                            const struct ib_cq_init_attr *init_attr,
                            struct ib_udata *udata);

int dis_poll_cq(struct ib_cq *ibcq, int num_wc, struct ib_wc *ibwc);
int dis_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags flags);
void dis_destroy_cq(struct ib_cq *ibcq, struct ib_udata *udata);

// Queue pair verbs.
struct ib_qp *dis_create_qp(struct ib_pd *ibpd,
                            struct ib_qp_init_attr *init_attr,
                            struct ib_udata *udata);
int dis_query_qp(struct ib_qp *ibqp, 
                    struct ib_qp_attr *attr,
                    int attr_mask,
                    struct ib_qp_init_attr *init_attr);
int dis_modify_qp(struct ib_qp *ibqp, 
                    struct ib_qp_attr *attr,
                    int attr_mask, 
                    struct ib_udata *udata);
int dis_destroy_qp(struct ib_qp *ibqp, struct ib_udata *udata);
int dis_post_send(struct ib_qp *ibqp, 
                    const struct ib_send_wr *send_wr,
                    const struct ib_send_wr **bad_wr);
int dis_post_recv(struct ib_qp *ibqp, 
                    const struct ib_recv_wr *recv_wr,
                    const struct ib_recv_wr **bad_wr);

// Shared Receive Queue verbs.
int dis_create_srq(struct ib_srq *ibsrq, 
                    struct ib_srq_init_attr *init_attr, 
                    struct ib_udata *udata);
int dis_modify_srq(struct ib_srq *ibsrq, 
                    struct ib_srq_attr *attr, 
                    enum ib_srq_attr_mask attr_mask, 
                    struct ib_udata *udata);
int dis_query_srq(struct ib_srq *ibsrq, struct ib_srq_attr *attr);
int dis_post_srq_recv(struct ib_srq *ibsrq, 
                        const struct ib_recv_wr *wr,
                        const struct ib_recv_wr **bad_wr);
void dis_destroy_srq(struct ib_srq *ibsrq,  struct ib_udata *udata);


// Address Handle verbs.
// int dis_create_ah(struct ib_ah *ibah,
//                             struct rdma_ah_attr *ah_attr,
//                             u32 flags,
//                             struct ib_udata *udata);
// void dis_destroy_ah(struct ib_ah *ibah, u32 flags);

#endif /* __DIS_VERBS_H__ */