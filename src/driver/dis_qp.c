#include "pr_fmt.h"

#include "dis_verbs.h"
#include "dis_qp.h"
#include "sci_if.h"

int dis_wq_post_cqe(struct dis_wq *wq, 
                    struct dis_wqe *wqe,
                    enum ib_wc_status wq_status) 
{
    unsigned long flags;
    struct dis_cq* cq = wq->cq;
    struct dis_cqe* cqe;
    pr_devel(DIS_STATUS_START);

    // TODO: Make an individual cqe lock?
    spin_lock_irqsave(&cq->cqe_lock, flags);

    cqe = cq->cqe_queue + (cq->cqe_put % cq->cqe_max);

    if(cqe->valid) {
        spin_unlock_irqrestore(&cq->cqe_lock, flags);
        return -42;
    }

    // cqe->ibqp       = &wq->qp->ibqp;
    cqe->id         = wqe->id;
    cqe->opcode     = wqe->opcode;
    cqe->byte_len   = wqe->byte_len;
    cqe->status     = wq_status;
    cqe->valid      = 1;

    cq->cqe_put++;

    spin_unlock_irqrestore(&cq->cqe_lock, flags);
    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

enum ib_wc_status dis_wq_consume_one_rqe(struct sci_if_v_msg *msg)
{
    int ret, size_left;
    pr_devel(DIS_STATUS_START);

    size_left = *(msg->size);
    while (!kthread_should_stop()) {
        ret = sci_if_receive_v_msg(msg);
        if (!ret) {
            // Note: msg->size is only changed on a successful, but partial, receive
            size_left -= *(msg->size);
            if(size_left == 0) {
                pr_devel(DIS_STATUS_COMPLETE);
                return IB_WC_SUCCESS;
            }
            *(msg->size) = size_left;
        }
    }

    pr_devel(DIS_STATUS_FAIL);
    return IB_WC_RESP_TIMEOUT_ERR;
}

enum ib_wc_status dis_wq_consume_one_sqe(struct sci_if_v_msg *msg)
{
    int ret;
    pr_devel(DIS_STATUS_START);

    while (!kthread_should_stop()) {
        ret = sci_if_send_v_msg(msg);
        if (!ret) {
            pr_devel(DIS_STATUS_COMPLETE);
            return IB_WC_SUCCESS;
        }
    }

    pr_devel(DIS_STATUS_FAIL);
    return IB_WC_RESP_TIMEOUT_ERR;
}

int dis_wq_consume_all(struct dis_wq *wq)
{
    int i, size, free;
    struct dis_wqe *wqe;
    struct sci_if_v_msg msg;
    struct iovec iov[DIS_WQE_SGE_MAX];
    enum ib_wc_status wc_status;
    pr_devel(DIS_STATUS_START);

    while (!kthread_should_stop()) {
        wqe = wq->wqe_queue + (wq->wqe_get % wq->wqe_max);

        if(!wqe->valid) {
            pr_devel(DIS_STATUS_COMPLETE);
            return 0;
        }

        msg.msq             = &wq->msq.msq;
        msg.size            = &size;
        msg.free            = &free;

        msg.msg.cmsg_valid  = 0;
        msg.msg.page        = NULL;
        msg.msg.iov         = iov;
        msg.msg.iovlen      = wqe->sge_count;
        
        //TODO: Move to post_send/post_receive
        size = 0;
        for (i = 0; i < wqe->sge_count; i++) {
            iov[i].iov_base = (void *)(wqe->sg_list[i].addr);
            iov[i].iov_len  = (size_t)(wqe->sg_list[i].length);
            size            += wqe->sg_list[i].length;
        }

        switch (wq->wq_type) {
        case DIS_RQ:
            wc_status = dis_wq_consume_one_rqe(&msg);
            break;

        case DIS_SQ:
            wc_status = dis_wq_consume_one_sqe(&msg);
            break;

        default:
            pr_devel(DIS_STATUS_FAIL);
            return -42;
        }

        dis_wq_post_cqe(wq, wqe, wc_status);
        wqe->valid = 0;
        wq->wqe_get++;
    }

    pr_devel(DIS_STATUS_FAIL);
    return -42;
}

int dis_wq_init(struct dis_wq *wq)
{
    int ret, sleep_ms;
    pr_devel(DIS_STATUS_START);

    sleep_ms = DIS_QP_SLEEP_MS_INI;
    while (!kthread_should_stop()) {
        switch (wq->wq_type) {
        case DIS_RQ:
            ret = sci_if_create_msq(&wq->msq);
            if(!ret) {
                pr_devel(DIS_STATUS_COMPLETE);
                return 0;
            }
            break;

        case DIS_SQ:
            ret = sci_if_connect_msq(&wq->msq);
            if(!ret) {
                pr_devel(DIS_STATUS_COMPLETE);
                return 0;
            }
            break;

        default:
            pr_devel(DIS_STATUS_FAIL);
            return -42;
        }

        sleep_ms = min(sleep_ms + DIS_QP_SLEEP_MS_INC, DIS_QP_SLEEP_MS_MAX);
        msleep_interruptible(sleep_ms);
    }

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

void dis_wq_exit(struct dis_wq *wq)
{
    pr_devel(DIS_STATUS_START);

    switch (wq->wq_type) {
    case DIS_RQ:
        sci_if_remove_msq(&wq->msq);
        break;

    case DIS_SQ:
        sci_if_disconnect_msq(&wq->msq);
        break;

    default:
        pr_devel(DIS_STATUS_FAIL);
        return;
    }

    pr_devel(DIS_STATUS_COMPLETE);
}

int dis_wq_thread(void *wq_buf)
{
    int ret, signal;
    struct dis_wq *wq = (struct dis_wq*)wq_buf;
    pr_devel(DIS_STATUS_START);
    
    wq->wq_state = DIS_WQ_RUNNING;
    ret = dis_wq_init(wq);
    if (ret) {
        wq->wq_state = DIS_WQ_EXITED;
        pr_devel(DIS_STATUS_FAIL);
        return 0;
    }
 
    while (!kthread_should_stop()) {
        signal = wait_event_killable(wq->wait_queue,
                                        wq->wq_flag != DIS_WQ_EMPTY ||
                                        kthread_should_stop());
        if (signal) {
            pr_devel("Kill signal received, exiting!");
            break;
        }

        if (kthread_should_stop()) {
            pr_devel("Kernel thread should stop, exiting!");
            break;
        }

        ret = dis_wq_consume_all(wq);
        if (ret) {
            break;
        }

        wq->wq_flag = DIS_WQ_EMPTY;
    }

    dis_wq_exit(wq);
    wq->wq_state = DIS_WQ_EXITED;
    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_qp_init(struct dis_wq *wq)
{
    pr_devel(DIS_STATUS_START);
    
    init_waitqueue_head(&wq->wait_queue);
    wq->wq_state = DIS_WQ_INITIALIZED;
    wq->wq_flag  = DIS_WQ_EMPTY;

    wq->thread = kthread_create(dis_wq_thread, (void*)wq, "DIS WQ Thread");//TODO: More descriptive name
    if (!wq->thread) {
        pr_devel(DIS_STATUS_FAIL);
        wq->wq_state = DIS_WQ_UNINITIALIZED;
        return -42;
    }
    
    wake_up_process(wq->thread);

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

int dis_qp_notify(struct dis_wq *wq)
{
    pr_devel(DIS_STATUS_START);

    if (wq->wq_state == DIS_WQ_UNINITIALIZED) {
        pr_devel(DIS_STATUS_FAIL);
        return -42;
    }

    wq->wq_flag = DIS_WQ_POST;
    wake_up(&wq->wait_queue);
    //TODO: Should we yield here?

    pr_devel(DIS_STATUS_COMPLETE);
    return 0;
}

void dis_qp_exit(struct dis_wq *wq)
{
    pr_devel(DIS_STATUS_START);

    if (wq->wq_state == DIS_WQ_UNINITIALIZED) {
        pr_devel(DIS_STATUS_COMPLETE);
        return;
    }

    kthread_stop(wq->thread);
    wake_up(&wq->wait_queue);
    wake_up_process(wq->thread);

    pr_devel(DIS_STATUS_COMPLETE);
}