#ifndef __DIS_VERBS_H__
#define __DIS_VERBS_H__

#include <rdma/ib_verbs.h>

// Device verbs.
int dis_query_device(struct ib_device *ibdev, struct ib_device_attr *props,
                     struct ib_udata *udata);
int dis_query_port(struct ib_device *ibdev, u8 port,
                   struct ib_port_attr *props);

// Protection Domain verbs.
int dis_alloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);
void dis_dealloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);

// Memory Region verbs.
// struct ib_mr *dis_reg_mr(struct ib_pd *ibpd, u64 start, u64 length,
//                          u64 virt_addr, int access_flags,
//                          struct ib_udata *udata);
int dis_dereg_mr(struct ib_mr *ibmr, struct ib_udata *udata);

// Completion Queue verbs.
int dis_create_cq(struct ib_cq *ibcq,
                            const struct ib_cq_init_attr *attr,
                            struct ib_udata *udata);
int dis_poll_cq(struct ib_cq *ibcq, int nwc, struct ib_wc *wc);
void dis_destroy_cq(struct ib_cq *ibcq, struct ib_udata *udata);

// Queue pair verbs.
struct ib_qp *dis_create_qp(struct ib_pd *ibpd,
                            struct ib_qp_init_attr *init_attr,
                            struct ib_udata *udata);
int dis_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *qp_attr,
                 int qp_attr_mask,
                 struct ib_qp_init_attr *qp_init_attr);
int dis_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *qp_attr,
                  int qp_attr_mask, struct ib_udata *udata);
int dis_destroy_qp(struct ib_qp *ibqp, struct ib_udata *udata);

// Posting verbs.
int dis_post_send(struct ib_qp *ibqp, const struct ib_send_wr *send_wr,
                  const struct ib_send_wr **bad_wr);
int dis_post_recv(struct ib_qp *ibqp, const struct ib_recv_wr *recv_wr,
                  const struct ib_recv_wr **bad_wr);

// Address Handle verbs.
int dis_create_ah(struct ib_ah *ibah,
                            struct rdma_ah_attr *ah_attr,
                            u32 flags,
                            struct ib_udata *udata);
void dis_destroy_ah(struct ib_ah *ibah, u32 flags);

#endif /* __DIS_VERBS_H__ */