#include "pr_fmt.h"

#include "dis_verbs.h"
#include "dis_qp.h"

static int global_qpn = 100;

int dis_query_device(struct ib_device *ibdev, struct ib_device_attr *dev_attr,
                        struct ib_udata *udata)
{
    pr_devel(DIS_STATUS_START);

    dev_attr->fw_ver               = 1;
    dev_attr->sys_image_guid       = 1234;
    dev_attr->max_mr_size          = ~0ull;
    dev_attr->page_size_cap        = 0xffff000; // 4KB-128MB
    dev_attr->vendor_id            = 1234;
    dev_attr->vendor_part_id       = 1234;
    dev_attr->hw_ver               = 1;
    dev_attr->max_qp               = 1234;
    dev_attr->max_qp_wr            = 1234;
    dev_attr->device_cap_flags     = IB_DEVICE_PORT_ACTIVE_EVENT;
    dev_attr->device_cap_flags     |= IB_DEVICE_LOCAL_DMA_LKEY;
    dev_attr->device_cap_flags     |= IB_DEVICE_MEM_MGT_EXTENSIONS;  // Support FR
    dev_attr->max_send_sge         = 1234;
    dev_attr->max_recv_sge         = 1234;
    dev_attr->max_sge_rd           = 1;
    dev_attr->max_cq               = 1234;
    dev_attr->max_cqe              = 1234;
    dev_attr->max_mr               = 1234;
    dev_attr->max_pd               = 1234;
    dev_attr->max_qp_rd_atom       = 0;
    dev_attr->max_qp_init_rd_atom  = 0;
    dev_attr->atomic_cap           = IB_ATOMIC_NONE;
    dev_attr->max_pkeys            = 1;
    dev_attr->local_ca_ack_delay   = 1;

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_query_port(struct ib_device *ibdev, u8 port,
                    struct ib_port_attr *port_attr)
{
    pr_devel(DIS_STATUS_START);
    //port_attr->port_cap_flags   = IB_PORT_CM_SUP;
    // port_attr->port_cap_flags   = IB_PORT_REINIT_SUP;
    // port_attr->port_cap_flags   |= IB_PORT_DEVICE_MGMT_SUP;
    // port_attr->port_cap_flags   |= IB_PORT_VENDOR_CLASS_SUP;
    port_attr->gid_tbl_len      = 1;
    port_attr->pkey_tbl_len     = 1;
    port_attr->max_vl_num       = 1;
    port_attr->max_msg_sz       = 0x80000000;
    port_attr->max_mtu          = 4096;
    port_attr->active_mtu       = 4096;
    port_attr->lid              = 0;
    port_attr->sm_lid           = 0;
    port_attr->bad_pkey_cntr    = 0;
    port_attr->qkey_viol_cntr   = 0;
    port_attr->lmc              = 0;
    port_attr->sm_sl            = 0;
    port_attr->subnet_timeout   = 0;
    port_attr->init_type_reply  = 0;

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_get_port_immutable(struct ib_device *ibdev, u8 port_num,
                            struct ib_port_immutable *immutable)
{
    pr_devel(DIS_STATUS_START);
    //int ret;
    // struct ib_port_attr port_attr;
    // ret = dis_query_port(ibdev, port_num, &port_attr);
    // if(ret) {
    //     return -42;
    // }
    
    // TODO: Replace hard-coded values with result from dis_query_port
    immutable->pkey_tbl_len     = 1;
    immutable->gid_tbl_len      = 1;
    immutable->core_cap_flags   = RDMA_CORE_PORT_RAW_PACKET;
    
    // This has to be 0 in order to not trigger the verify_immutable check
    immutable->max_mad_size = 0; //IB_MGMT_MAD_SIZE;
    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_query_pkey(struct ib_device *ibdev, u8 port, u16 index,
                    u16 *pkey)
{
    pr_devel(DIS_STATUS_START);

    *pkey = 0xffff;
    
    pr_devel(DIS_STATUS_COMPLETE);
    return 0;   
}

int dis_alloc_pd(struct ib_pd *ibpd, struct ib_udata *udata)
{
    struct dis_pd *dispd    = to_dis_pd(ibpd);
    struct ib_device *ibdev = ibpd->device;
    struct dis_dev *dev  = to_dis_dev(ibdev);
    pr_devel(DIS_STATUS_START);

    dispd->dev = dev;

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

void dis_dealloc_pd(struct ib_pd *ibpd, struct ib_udata *udata)
{
    pr_devel(DIS_STATUS_START);
    // Nothing to do.
    pr_devel(DIS_STATUS_COMPLETE);
}

struct ib_mr *dis_get_dma_mr(struct ib_pd *ibpd, int access)
{
    struct dis_mr *mr;
    struct ib_device *ibdev = ibpd->device;
    struct dis_dev *dev  = to_dis_dev(ibdev);
    pr_devel(DIS_STATUS_START);

    mr = kzalloc(sizeof(struct dis_mr), GFP_KERNEL);
    if (!mr) {
        dev_err(&ibdev->dev, "dis_get_dma_mr " DIS_STATUS_FAIL);
        return ERR_PTR(-1);
    }

    mr->dev = dev;

    pr_devel(DIS_STATUS_COMPLETE);
    return &mr->ibmr;
}

int dis_dereg_mr(struct ib_mr *ibmr, struct ib_udata *udata)
{
    struct dis_mr *mr = to_dis_mr(ibmr);

    pr_devel(DIS_STATUS_START);

    //TODO: Reuse memory?
    kfree(mr);

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_create_cq(struct ib_cq *ibcq, const struct ib_cq_init_attr *init_attr,
                    struct ib_udata *udata)
{
    struct dis_cq *cq    = to_dis_cq(ibcq);
    struct ib_device *ibdev = ibcq->device;
    struct dis_dev *dev  = to_dis_dev(ibdev);
    pr_devel(DIS_STATUS_START);

    cq->dev       = dev;
    cq->cqe_get      = 0;
    cq->cqe_put      = 0;
    cq->cqe_max      = roundup_pow_of_two(init_attr->cqe);
    cq->cqe_queue    = kzalloc(sizeof(struct dis_cqe) * cq->cqe_max, 
                                GFP_KERNEL);
    if (!cq->cqe_queue) {
        pr_devel(DIS_STATUS_FAIL);
        return -42;
    }
    memset(cq->cqe_queue, 0, sizeof(struct dis_cqe) * cq->cqe_max);

    ibcq->cqe = cq->cqe_max;

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_poll_cq(struct ib_cq *ibcq, int num_wc, struct ib_wc *ibwc)
{
    int wc_count;
    unsigned long flags;
    struct dis_cq *cq = to_dis_cq(ibcq);
    struct dis_cqe* cqe;
    struct ib_wc *ibwc_iter;
    pr_devel(DIS_STATUS_START);

    ibwc_iter = ibwc;
    for (wc_count = 0; wc_count < num_wc; wc_count++) {
        spin_lock_irqsave(&cq->cqe_lock, flags);

        cqe = cq->cqe_queue + (cq->cqe_get % cq->cqe_max);

        if(cqe->flags != DIS_WQE_VALID) {
            spin_unlock_irqrestore(&cq->cqe_lock, flags);
            break;
        }
        ibwc_iter->wr_id    = cqe->id;
        ibwc_iter->status   = cqe->status;
        ibwc_iter->opcode   = cqe->opcode;
        ibwc_iter->byte_len = cqe->byte_len;
        ibwc_iter->qp       = cqe->ibqp;

        cqe->flags = DIS_CQE_FREE;
        cq->cqe_get++;
        spin_unlock_irqrestore(&cq->cqe_lock, flags);
        ibwc_iter++;
    }
    
    pr_devel(DIS_STATUS_COMPLETE);
    return wc_count;
}

int dis_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags flags)
{
    pr_devel(DIS_STATUS_START);

    pr_devel(DIS_STATUS_FAIL);
    return -42;
}

void dis_destroy_cq(struct ib_cq *ibcq, struct ib_udata *udata)
{
    struct dis_cq *cq = to_dis_cq(ibcq);
    pr_devel(DIS_STATUS_START);

    kfree(cq->cqe_queue);

    pr_devel(DIS_STATUS_COMPLETE);
}

struct ib_qp *dis_create_qp(struct ib_pd *ibpd,
                            struct ib_qp_init_attr *init_attr,
                            struct ib_udata *udata)
{
    struct dis_qp *qp;
    pr_devel(DIS_STATUS_START);

    qp = kzalloc(sizeof(struct dis_qp), GFP_KERNEL);
    if (!qp) {
        goto create_qp_alloc_qp_err;
    }

    qp->dev           = to_dis_dev(ibpd->device);
    qp->sq_sig_type      = init_attr->sq_sig_type;
    qp->type             = init_attr->qp_type;
    qp->state            = IB_QPS_RESET;
    qp->mtu              = ib_mtu_enum_to_int(IB_MTU_4096);
    qp->l_qpn            = global_qpn++;
    qp->event_handler    = init_attr->event_handler;

    qp->ibqp.pd          = ibpd;
    qp->ibqp.send_cq     = init_attr->send_cq;
    qp->ibqp.recv_cq     = init_attr->recv_cq;
    qp->ibqp.srq         = init_attr->srq;
    qp->ibqp.qp_type     = init_attr->qp_type;
    qp->ibqp.qp_num      = qp->l_qpn;

    qp->sq.qp         = qp;
    qp->sq.cq         = to_dis_cq(init_attr->send_cq);
    qp->sq.max_sge       = init_attr->cap.max_send_sge;
    qp->sq.max_inline    = init_attr->cap.max_inline_data;
    qp->sq.wqe_get       = 0;
    qp->sq.wqe_put       = 0;
    qp->sq.wq_type       = DIS_SQ;
    qp->sq.wqe_max       = roundup_pow_of_two(init_attr->cap.max_send_wr);
    qp->sq.wqe_queue     = kzalloc(sizeof(struct dis_wqe) * qp->sq.wqe_max, 
                            GFP_KERNEL);
    if (!qp->sq.wqe_queue) {
        pr_devel(DIS_STATUS_FAIL);
        goto create_qp_alloc_sq_err;
    }
    memset(qp->sq.wqe_queue, 0, sizeof(struct dis_wqe) * qp->sq.wqe_max);

    qp->rq.qp         = qp;
    qp->rq.cq         = to_dis_cq(init_attr->recv_cq);
    qp->rq.max_sge       = init_attr->cap.max_recv_sge;
    qp->rq.max_inline    = init_attr->cap.max_inline_data;
    qp->rq.wqe_get       = 0;
    qp->rq.wqe_put       = 0;
    qp->rq.wq_type       = DIS_RQ;
    qp->rq.wqe_max       = roundup_pow_of_two(init_attr->cap.max_recv_wr);
    qp->rq.wqe_queue     = kzalloc(sizeof(struct dis_wqe) * qp->rq.wqe_max, 
                            GFP_KERNEL);
    if (!qp->rq.wqe_queue) {
        pr_devel(DIS_STATUS_FAIL);
        goto create_qp_alloc_rq_err;
    }
    memset(qp->rq.wqe_queue, 0, sizeof(struct dis_wqe) * qp->rq.wqe_max);
    
    return &qp->ibqp;

create_qp_alloc_rq_err:
    kfree(qp->sq.wqe_queue);

create_qp_alloc_sq_err:
    kfree(qp);

create_qp_alloc_qp_err:
    return ERR_PTR(-1);
}

int dis_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
                    int attr_mask,
                    struct ib_qp_init_attr *init_attr)
{
    pr_devel(DIS_STATUS_START);

    pr_devel(DIS_STATUS_FAIL);
    return -42;
}

int dis_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
                    int attr_mask, struct ib_udata *udata)
{
    int ret;
    struct dis_qp *qp = to_dis_qp(ibqp);
    pr_devel(DIS_STATUS_START);

    if (attr_mask & IB_QP_STATE) {
        switch (attr->qp_state) {
        case IB_QPS_RESET:
            pr_devel("Modify QP state: RESET");
            break;

        case IB_QPS_INIT:
            pr_devel("Modify QP state: INIT");

            qp->rq.msq.max_msg_count = 16;//qp->rq.wqe_max;
            qp->sq.msq.max_msg_count = 16;//qp->sq.wqe_max;
            qp->rq.msq.max_msg_size = 256*sizeof(char);//qp->mtu;
            qp->sq.msq.max_msg_size = 256*sizeof(char);//qp->mtu;
            break;

        case IB_QPS_RTR:
            pr_devel("Modify QP state: RTR");
            if (attr_mask & IB_QP_DEST_QPN) {
                // qp->r_qpn    = attr->dest_qp_num;
                qp->r_qpn = qp->l_qpn;

                qp->rq.msq.l_qpn = qp->l_qpn;
                qp->rq.msq.r_qpn = qp->r_qpn;
                qp->sq.msq.l_qpn = qp->l_qpn;
                qp->sq.msq.r_qpn = qp->r_qpn;
            }
            
            if (attr_mask & IB_QP_PATH_MTU) {
                qp->mtu = ib_mtu_enum_to_int(attr->path_mtu);

                // qp->rq.msq.max_msg_size = qp->mtu;
                // qp->sq.msq.max_msg_size = qp->mtu;
            }
            
            ret = dis_qp_init(&qp->rq);
            if(ret) {
                pr_devel(DIS_STATUS_FAIL);
                return -42;
            }
            break;

        case IB_QPS_RTS:
            pr_devel("Modify QP state: RTS");
            ret = dis_qp_init(&qp->sq);
            if(ret) {
                pr_devel(DIS_STATUS_FAIL);
                return -42;
            }
            break;

        default:
            pr_devel(DIS_STATUS_FAIL);
            return -42;
        }

        qp->state = attr->qp_state;
    }

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_destroy_qp(struct ib_qp *ibqp, struct ib_udata *udata)
{
    struct dis_qp *qp = to_dis_qp(ibqp);
    pr_devel(DIS_STATUS_START);

    dis_qp_exit(&qp->rq);
    dis_qp_exit(&qp->sq);
    kfree(qp->rq.wqe_queue);
    kfree(qp->sq.wqe_queue);
    kfree(qp);

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_post_send(struct ib_qp *ibqp, const struct ib_send_wr *send_wr,
                    const struct ib_send_wr **bad_wr)
{
    struct dis_qp *qp = to_dis_qp(ibqp);
    struct dis_wqe *sqe;
    const struct ib_send_wr *send_wr_iter;
    pr_devel(DIS_STATUS_START);

    send_wr_iter = send_wr;
    while (send_wr_iter) {
        sqe = qp->sq.wqe_queue + (qp->sq.wqe_put % qp->sq.wqe_max);

        if(sqe->flags != DIS_WQE_FREE) {
            pr_devel(DIS_STATUS_FAIL);
            return -42;
        }

        sqe->num_sge = min(send_wr_iter->num_sge, DIS_MAX_SGE);
        memcpy(sqe->sg_list, 
                send_wr_iter->sg_list, 
                sizeof(struct ib_sge) * sqe->num_sge);

        sqe->opcode = IB_WC_SEND;
        sqe->flags = DIS_WQE_VALID;
        qp->sq.wqe_put++;
        dis_qp_notify(&qp->sq);
        send_wr_iter = send_wr_iter->next;
    }
    
    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_post_recv(struct ib_qp *ibqp, const struct ib_recv_wr *recv_wr,
                    const struct ib_recv_wr **bad_wr)
{
    struct dis_qp *qp = to_dis_qp(ibqp);
    struct dis_wqe *rqe;
    const struct ib_recv_wr *recv_wr_iter;
    pr_devel(DIS_STATUS_START);

    recv_wr_iter = recv_wr;
    while (recv_wr_iter) {
        rqe = qp->rq.wqe_queue + (qp->rq.wqe_put % qp->rq.wqe_max);

        if(rqe->flags != DIS_WQE_FREE) {
            pr_devel(DIS_STATUS_FAIL);
            return -42;
        }

        rqe->num_sge = min(recv_wr_iter->num_sge, DIS_MAX_SGE);
        memcpy(rqe->sg_list,
                recv_wr_iter->sg_list,
                sizeof(struct ib_sge) * rqe->num_sge);

        rqe->opcode = IB_WC_RECV;
        rqe->flags = DIS_WQE_VALID;
        qp->rq.wqe_put++;
        dis_qp_notify(&qp->rq);
        recv_wr_iter = recv_wr_iter->next;
    }
    
    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}