#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>

//#include <linux/netdevice.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

#include "dis_main.h"
#include "dis_verbs.h"

#define MINOR_BASE  0
#define MINOR_COUNT	255

#define DIS_BUS_NAME 		"dis-bus"
#define DIS_BUS_DEV_NAME 	"dis-bus-device"
#define DIS_DRV_NAME 		"dis-device"

#define DIS_ROPCIE_DRV_VERSION "0.0"
#define DIS_ROPCIE_DRV_DESC "Dolphin Interconnect Soulutions RoPCIe Driver"

MODULE_DESCRIPTION(DIS_ROPCIE_DRV_DESC " " DIS_ROPCIE_DRV_VERSION);
MODULE_AUTHOR("Alve Elde");
MODULE_LICENSE("GPL");

static const struct ib_device_ops dis_dev_ops = {
	.owner = THIS_MODULE,
	.driver_id = RDMA_DRIVER_UNKNOWN,
	.uverbs_abi_ver = 1,

	.alloc_pd = dis_alloc_pd,
	.create_cq = dis_create_cq,
	.create_qp = dis_create_qp,
	.dealloc_pd = dis_dealloc_pd,
	.dereg_mr = dis_dereg_mr,
	.destroy_cq = dis_destroy_cq,
	.destroy_qp = dis_destroy_qp,
	.get_dma_mr = dis_get_dma_mr,
	.get_port_immutable = dis_get_port_immutable, 
	.modify_qp = dis_modify_qp,
	.poll_cq = dis_poll_cq,
	.post_recv = dis_post_recv,
	.post_send = dis_post_send,
	.query_pkey = dis_query_pkey,
	.query_port = dis_query_port,
	.query_qp = dis_query_qp,
	.req_notify_cq = dis_req_notify_cq, 
    .query_device = dis_query_device, 

	// INIT_RDMA_OBJ_SIZE(ib_ah, dis_ah, ibah),
	// INIT_RDMA_OBJ_SIZE(ib_cq, dis_cq, ibcq),
	// INIT_RDMA_OBJ_SIZE(ib_pd, dis_pd, ibpd),
	// INIT_RDMA_OBJ_SIZE(ib_ucontext, dis_ucontext, ibucontext),
};
//static struct dis_device *disdev;


// BUS

static int dis_bus_match(struct device *dev, struct device_driver *driver)
{
	printk(KERN_INFO "dis-bus match.\n");
	return !strncmp(dev_name(dev), driver->name, strlen(driver->name));	
}

struct bus_type disbus = {
    .name = DIS_BUS_NAME,
    .match = dis_bus_match,
};

static void dis_bus_dev_release(struct device *dev)
{
    printk(KERN_INFO "dis-bus-dev release.\n");
}

struct device disbusdev = {
    .init_name = DIS_BUS_DEV_NAME,
	.release = dis_bus_dev_release,
};

// DRIVER

static int dis_driver_probe(struct device *dev)
{
	printk(KERN_INFO "dis-dev probe.\n");
	return 0;
}

static int dis_driver_remove(struct device *dev)
{
	printk(KERN_INFO "dis-dev remove.\n");
	return 0;
}

struct device_driver disdrv = {
	.name = DIS_DRV_NAME,
	.bus = &disbus,
	.probe = dis_driver_probe,
	.remove = dis_driver_remove,
};

// DEVICE

static void dis_dev_release(struct device *dev)
{
    printk(KERN_INFO "dis-dev release.\n");
}

struct device disdev = {
	.init_name = DIS_DRV_NAME,
    .bus = &disbus,
	.parent = &disbusdev,
	.release = dis_dev_release,
};


// struct dis_device {
//     struct ib_device ibdev;
    
// 	struct device dev;
// 	dev_t first;
// 	struct class *class;
// };
// static struct dis_device disdev;


static int __init dis_init_module(void)
{
	int ret;

	// BUS
    ret = bus_register(&disbus);
    if(ret) {
        printk(KERN_INFO "bus_register failed!\n");
        return -1;
    }
    printk(KERN_INFO "dis-bus registered.\n");

	ret = device_register(&disbusdev);
    if (ret) {
		bus_unregister(&disbus);
		printk(KERN_INFO "bus device_register failed!\n");
        return -1;
    }
	printk(KERN_INFO "dis-bus-dev registered.\n");

	// DRIVER
	ret = driver_register(&disdrv);
	if(ret) {
		device_unregister(&disbusdev);
		bus_unregister(&disbus);
		printk(KERN_INFO "driver_register failed!\n");
		return -1;
	}
	printk(KERN_INFO "dis-driver registered.\n");

	//DEVICE
	ret = device_register(&disdev);
	if(ret) {
		driver_unregister(&disdrv);
		device_unregister(&disbusdev);
		bus_unregister(&disbus);
		printk(KERN_INFO "device_register failed!\n");
		return -1;
	}
	printk(KERN_INFO "dis-dev registered.\n");



	// disdev = ib_alloc_device(dis_device, ibdev);
	// if(!disdev) {
	// 	printk(KERN_INFO "ib_alloc_device failed!\n");
	// 	return -1;
	// }

	// disdev->ibdev.node_type = RDMA_NODE_IB_CA;
	// disdev->ibdev.phys_port_cnt = 1;
	// disdev->ibdev.local_dma_lkey = 1234;
	// disdev->ibdev.num_comp_vectors	= 1;
	// ib_set_device_ops(&(disdev->ibdev), &dis_dev_ops);
	

	//ret = register_netdevice_notifier()
	//ib_device_set_netdev(ibdev, netdev, port);

	//ret = ib_register_device(&(disdev->ibdev), "dis");
	// if(ret) {
	// 	printk(KERN_INFO "ib_register_device failed!\n");
	// 	ib_dealloc_device(ibdev);
	// 	return -1;
	// }

	printk(KERN_INFO "dis_init_module complete.\n");
    return 0;
}


static void __exit dis_exit_module(void)
{
	// DEVICE
	device_unregister(&disdev);
	printk(KERN_INFO "dis-dev unregistered.\n");

	// DRIVER
	driver_unregister(&disdrv);
	printk(KERN_INFO "dis-drv unregistered.\n");

	// BUS
	device_unregister(&disbusdev);
	printk(KERN_INFO "dis-bus-dev unregistered.\n");
	bus_unregister(&disbus);
	printk(KERN_INFO "dis-bus unregistered.\n");

	//ib_unregister_device(&(disdev->ibdev));
	//ib_dealloc_device(&(disdev->ibdev));
	printk(KERN_INFO "dis_exit_module complete.\n");
}

module_init(dis_init_module);
module_exit(dis_exit_module);
