/*
 * Driver for Pixcir I2C touchscreen controllers.
 *
 * Copyright (C) 2010-2012 Pixcir, Inc.
 *
 * pixcir_i2c_ts.c V3.0	from v3.0 support TangoC solution and remove the previous soltutions
 *
 * pixcir_i2c_ts.c V3.1	Add bootloader function	7
 *			Add RESET_TP		9
 * 			Add ENABLE_IRQ		10
 *			Add DISABLE_IRQ		11
 * 			Add BOOTLOADER_STU	16
 *			Add ATTB_VALUE		17
 *			Add Write/Read Interface for APP software
 *
 * pixcir_i2c_ts.c V3.2.09	for INT_MODE 0x09
 *			change to workqueue for ISR
 *			adaptable report rate self
 *
 * pixcir_i2c_ts.c V3.3.09
 * 			Add Android early power management
 *			Add irq_flag for pixcir debug tool
 *			Add CRC attb check when bootloader
 *
 * pixcir_i2c_ts.c V3.4.09
 * 			modify bootloader according to interrupt
 *          add pr_info function, show info when define PIXCIR_DEBUG
 *
 * pixcir_i2c_ts.c V3.4.2_INT09
 *          set pen event and report pen data to system
 *
 * pixcir_i2c_ts.c INT09_v3.4.3
 *          report pressure value from 0 to 1
 *          application layer display pen's pressure must be multiplied by 512
 *
 * This code is proprietary and is subject to license terms.
 *
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
    #include <linux/earlysuspend.h>
#endif

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/gpio.h> 
#include <mach/sys_config.h>
#include <linux/init-input.h>


/*********************************Bee-0928-TOP****************************************/
//#define PIXCIR_DEBUG

#define SLAVE_ADDR			0x5c
#define	BOOTLOADER_ADDR		0x5d

#ifndef I2C_MAJOR
#define I2C_MAJOR 			125
#endif

#define I2C_MINORS 			256

#define	CALIBRATION_FLAG	1
#define	BOOTLOADER			7
#define RESET_TP			9

#define	ENABLE_IRQ			10
#define	DISABLE_IRQ			11
#define	BOOTLOADER_STU		16
#define ATTB_VALUE			17

#define	MAX_FINGER_NUM		5

static unsigned char status_reg = 0;
volatile int irq_num, irq_flag, work_pending = 0;

struct i2c_dev {
	struct list_head list;
	struct i2c_adapter *adap;
	struct device *dev;
};

static struct i2c_driver pixcir_i2c_ts_driver;
static struct class *i2c_dev_class;
static LIST_HEAD( i2c_dev_list);
static DEFINE_SPINLOCK( i2c_dev_list_lock);

static struct workqueue_struct *pixcir_wq;

struct pixcir_i2c_ts_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	int irq_is_disable; /* 0: irq enable */
	spinlock_t irq_lock;
};

struct point_node_t {
	unsigned char active;
	unsigned char finger_id;
	unsigned int posx;
	unsigned int posy;
};

static struct point_node_t point_slot[MAX_FINGER_NUM * 2];
static int bootloader_delay = 1500 * 5;
volatile int bootloader_irq = 0; // 0: no, 1: falling
static const int BTIME = 200;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void pixcir_ts_early_suspend(struct early_suspend *h);
static void pixcir_ts_late_resume(struct early_suspend *h);
#endif

/* ----------------------------------------------------------------------------------- */

#ifdef PRINT_INT_INFO
#define print_int_info(fmt, args...)     \
        do{                              \
                pr_info(fmt, ##args);     \
        }while(0)
#else
#define print_int_info(fmt, args...)   //
#endif

static __u32 twi_id = 0;
static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;

static u32 int_handle = 0;
static const unsigned short normal_i2c[2] = {0x5c, I2C_CLIENT_END};

struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
};

#define CTP_NAME		"pixcir_ts"
#define CTP_IRQ_MODE	(TRIG_EDGE_NEGATIVE)
#define GTP_RST_GPIO 	(config_info.wakeup_number)
#define GTP_INT_GPIO	(config_info.int_number)
#define SCREEN_MAX_X	(screen_max_x)
#define SCREEN_MAX_Y	(screen_max_y)
#define PRESS_MAX		(255)

/**
 * ctp_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	pr_info("%s, twi_id = %d, adapter->nr = %d", __func__, twi_id, adapter->nr);
      
    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)){
    	pr_err("I2C check functionality failed");
        return -ENODEV;
    }

	pr_info("%s, twi_id = %d, adapter->nr = %d", __func__, twi_id, adapter->nr);
        
    if(twi_id == adapter->nr){
    	pr_info("%s: Detected chip %s at adapter %d, address 0x%02x\n",
			 __func__, CTP_NAME, i2c_adapter_id(adapter), client->addr);         	    

		strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
		return 0;	
	}else{
	    return -ENODEV;
	}
}

static __s32 ctp_gpio_get_status(u32 gpio)
{
	return __gpio_get_value(gpio);
}

static void ctp_gpio_as_input(u32 gpio)
{
	int gpio_status = -1;
        
    gpio_status = sw_gpio_getcfg(gpio);
    if(gpio_status != 0){
    	sw_gpio_setcfg(gpio, 0);
    }
}

static void ctp_gpio_as_output(u32 gpio, int value)
{
	int gpio_status = -1;
        
    gpio_status = sw_gpio_getcfg(gpio);
    if(gpio_status != 1){
    	sw_gpio_setcfg(gpio, 1);
    }

	__gpio_set_value(gpio, value);

}

#if 0
static void ctp_gpio_as_int(u32 gpio)
{
    struct gpio_config_eint_all cfg = {0};

    /* config to eint, enable the eint, and set pull, drivel level, trig type */
    cfg.gpio        = gpio;
    cfg.pull        = GPIO_PULL_DEFAULT;
    cfg.drvlvl      = GPIO_DRVLVL_DEFAULT;
    cfg.enabled     = 0;
    cfg.trig_type   = CTP_IRQ_MODE;
	
    if (sw_gpio_eint_setall_range(&cfg, 1)) {
        pr_err("%s: gpio %d eint set all range failed\n", __func__, gpio);
        return;
    }
}
#endif

static void ctp_gpio_free(u32 gpio)
{
	gpio_free(gpio);
}

static int ctp_get_attb_value(void)
{
	return ctp_gpio_get_status(GTP_INT_GPIO);
}

static void ctp_irq_disable(struct pixcir_i2c_ts_data *tsdata)
{
    unsigned long irqflags;

    spin_lock_irqsave(&tsdata->irq_lock, irqflags);
    if (!tsdata->irq_is_disable)
    {
        tsdata->irq_is_disable = 1; 
		sw_gpio_eint_set_enable(GTP_INT_GPIO, 0);
    }
    spin_unlock_irqrestore(&tsdata->irq_lock, irqflags);
}

static void ctp_irq_enable(struct pixcir_i2c_ts_data *tsdata)
{
    unsigned long irqflags = 0;

    spin_lock_irqsave(&tsdata->irq_lock, irqflags);
    if (tsdata->irq_is_disable) 
    {
		sw_gpio_eint_set_enable(GTP_INT_GPIO, 0);
        tsdata->irq_is_disable = 0; 
    }
    spin_unlock_irqrestore(&tsdata->irq_lock, irqflags);
}

static void return_i2c_dev(struct i2c_dev *i2c_dev) {
	spin_lock(&i2c_dev_list_lock);
	list_del(&i2c_dev->list);
	spin_unlock(&i2c_dev_list_lock);
	kfree(i2c_dev);
}

static struct i2c_dev *i2c_dev_get_by_minor(unsigned index) {
	struct i2c_dev *i2c_dev;
	i2c_dev = NULL;

	spin_lock(&i2c_dev_list_lock);
	list_for_each_entry(i2c_dev, &i2c_dev_list, list)
	{
		if (i2c_dev->adap->nr == index) goto found;
	}
	i2c_dev = NULL;
	found: spin_unlock(&i2c_dev_list_lock);
	return i2c_dev;
}

static struct i2c_dev *get_free_i2c_dev(struct i2c_adapter *adap) {
	struct i2c_dev *i2c_dev;

	if (adap->nr >= I2C_MINORS) {
		printk(KERN_ERR "i2c-dev: Out of device minors (%d)\n",
				adap->nr);
		return ERR_PTR(-ENODEV);
	}

	i2c_dev = kzalloc(sizeof(*i2c_dev), GFP_KERNEL);
	if (!i2c_dev) return ERR_PTR(-ENOMEM);

	i2c_dev->adap = adap;

	spin_lock(&i2c_dev_list_lock);
	list_add_tail(&i2c_dev->list, &i2c_dev_list);
	spin_unlock(&i2c_dev_list_lock);
	return i2c_dev;
}

static void pixcir_ts_poscheck(struct work_struct *work) {
	struct pixcir_i2c_ts_data *tsdata =
			container_of(work, struct pixcir_i2c_ts_data, work);
	unsigned char *p;
	unsigned char touch, button, pix_id, slot_id;
	unsigned char rdbuf[32], wrbuf[1] = { 0 };
	int ret, i, pressure=0;
	static unsigned char type = 0;

	if (work_pending == 2) 
		goto exit_work_func;
	
	ret = i2c_master_send(tsdata->client, wrbuf, sizeof(wrbuf));
	if (ret != sizeof(wrbuf)) {
		dev_err(&tsdata->client->dev, "%s: i2c_master_send failed(), ret=%d\n",
		        __func__, ret);
	}

	ret = i2c_master_recv(tsdata->client, rdbuf, sizeof(rdbuf));
	if (ret != sizeof(rdbuf)) {
		dev_err(&tsdata->client->dev, "%s: i2c_master_recv() failed, ret=%d\n",
		        __func__, ret);
	}

	touch = rdbuf[0] & 0x07;
	button = rdbuf[1];

	//printk("touch=%d,button=%d\n", touch, button);

#ifdef BUTTON
	if (button) {
		switch (button) {
		case 1:
			input_report_key(tsdata->input, KEY_HOME, 1);
		case 2:
			//add other key down report
		case 4:
		case 8:
			//add case for other more key 
		default:
			break;
		}
	}
	else {
		input_report_key(tsdata->input, KEY_HOME, 0);
		//add other key up report
	}
#endif

	p = &rdbuf[2];
	if (!type) type = 1;
	for (i = 0; i < touch; i++) {
		pix_id = (*(p + 4));
		if (pix_id >= 254) { // pen
			type = 2;
			point_slot[0].posx = (*(p + 1) << 8) + (*(p));
			point_slot[0].posy = (*(p + 3) << 8) + (*(p + 2));
			pressure = (*(p + 26) << 8) + (*(p + 25));
		}
		else { // finger
			slot_id = ((pix_id & 7) << 1) | ((pix_id & 8) >> 3);
			point_slot[slot_id].active = 1;
			point_slot[slot_id].finger_id = pix_id;
			point_slot[slot_id].posx = (*(p + 1) << 8) + (*(p));
			point_slot[slot_id].posy = (*(p + 3) << 8) + (*(p + 2));
			p += 5;
		}
	}

	if (touch) {
		
		if (type == 1) { // finger
			//input_report_key(tsdata->input, BTN_TOOL_FINGER, 1);
			//input_report_abs(tsdata->input, ABS_MT_TOUCH_MAJOR, 15);
			for (i = 0; i < MAX_FINGER_NUM * 2; i++) {
				if (point_slot[i].active == 1) {
					point_slot[i].active = 0;
					input_report_key(tsdata->input, BTN_TOUCH, 1);
					input_report_abs(tsdata->input, ABS_MT_TRACKING_ID, i);
					input_report_abs(tsdata->input, ABS_MT_TOUCH_MAJOR, 15);
					input_report_abs(tsdata->input, ABS_MT_POSITION_X, point_slot[i].posx);
					input_report_abs(tsdata->input, ABS_MT_POSITION_Y, point_slot[i].posy);
					input_report_abs(tsdata->input, ABS_MT_PRESSURE, 1);
					input_mt_sync(tsdata->input);

					pr_info("slot=%d, x%d=%d, y%d=%d, touch=%d\n", i, i / 2,
					        point_slot[i].posx, i / 2, point_slot[i].posy, touch);
				}
			}
		}
		else { // pen
			input_report_key(tsdata->input, BTN_TOOL_PEN, 1);
			input_report_key(tsdata->input, BTN_TOUCH, 1);
			//input_report_key(tsdata->input, ABS_MT_TRACKING_ID, 0);
			input_report_abs(tsdata->input, ABS_MT_POSITION_X, point_slot[0].posx);
			input_report_abs(tsdata->input, ABS_MT_POSITION_Y, point_slot[0].posy);
			input_report_abs(tsdata->input, ABS_MT_PRESSURE, pressure);
			input_mt_sync(tsdata->input);
			pr_info("pen slot=%d, x%d=%d, y%d=%d, touch=%d\n", i, i / 2,
					 point_slot[i].posx, i / 2, point_slot[i].posy, touch);
		}
	}
	else {
		input_report_key(tsdata->input, BTN_TOUCH, 0);
		input_report_key(tsdata->input, BTN_TOOL_FINGER, 0);
		input_report_key(tsdata->input, BTN_TOOL_PEN, 0);
		input_report_abs(tsdata->input, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(tsdata->input, ABS_MT_PRESSURE, 0);
		type = 0;
	}

	input_sync(tsdata->input);

exit_work_func:
    //ctp_irq_enable(tsdata);
    return;
}

static u32 pixcir_ts_isr(struct pixcir_i2c_ts_data *tsdata) 
{
	//pr_info("%s, work_pending=%d\n", __func__, work_pending);

	if (work_pending == 1) { 
		//ctp_irq_disable(tsdata);
		queue_work(pixcir_wq, &tsdata->work);
	}
	else if (work_pending == 2) { // bootloader
		bootloader_irq = 1;
		
	}

	return 0;
}

static void pixcir_reset(void)
{
	/* reset */
	ctp_gpio_as_output(GTP_RST_GPIO, 1);
	mdelay(10);
	ctp_gpio_as_output(GTP_RST_GPIO, 0);
}

static int pixcir_ts_request_irq(struct pixcir_i2c_ts_data *tsdata)
{
	pr_info("%s\n", __func__);
	
	int_handle = sw_gpio_irq_request(GTP_INT_GPIO,CTP_IRQ_MODE,(peint_handle)pixcir_ts_isr, tsdata);
	if (!int_handle) {
		pr_err( "%s: request irq failed\n", __func__);
		ctp_gpio_as_input(GTP_INT_GPIO);
        ctp_gpio_free(GTP_INT_GPIO);
	}
	else 
    {
    	ctp_set_int_port_rate(config_info.int_number, 1);
		ctp_set_int_port_deb(config_info.int_number, 0x07);
        //ctp_irq_disable(tsdata);
    }
	
	return 0;
}

static int __devinit pixcir_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct pixcir_i2c_ts_data *tsdata;
	struct input_dev *input;
	struct device *dev;
	struct i2c_dev *i2c_dev;
	int i, ret;

	pr_info("%s\n", __func__);

	//pixcir_reset();

	for(i=0; i<MAX_FINGER_NUM*2; i++) {
		point_slot[i].active = 0;
	}

	tsdata = kzalloc(sizeof(*tsdata), GFP_KERNEL);
	input = input_allocate_device();
	if (!tsdata || !input) {
		dev_err(&client->dev, "Failed to allocate driver data!\n");
		ret = -ENOMEM;
		goto err_free_mem;
	}

	tsdata->client = client;
	tsdata->input = input;

	INIT_WORK(&tsdata->work, pixcir_ts_poscheck);
	spin_lock_init(&tsdata->irq_lock);
	
	set_bit(EV_KEY, input->evbit);
	set_bit(EV_ABS, input->evbit);
	set_bit(EV_SYN, input->evbit);
	
	set_bit(BTN_TOUCH, input->keybit);
	set_bit(BTN_TOOL_FINGER, input->keybit);
	set_bit(BTN_TOOL_PEN, input->keybit); 

	/* set device type to touchscreen, otherwise will be recongnized as a pointer device */
	set_bit(INPUT_PROP_DIRECT, input->propbit);

	input_set_abs_params(input, ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, 255, 0, 0);
	
	input->name = CTP_NAME;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;
	input_set_drvdata(input, tsdata);

	ret = input_register_device(input);
	if (ret)
		goto err_free_irq;

	i2c_set_clientdata(client, tsdata);

	work_pending = 1;
	irq_flag = 1;
	
	ret = pixcir_ts_request_irq(tsdata);
	if (ret)
		goto err_free_mem;
	//device_init_wakeup(&client->dev, 1);

	i2c_dev = get_free_i2c_dev(client->adapter);
	if (IS_ERR(i2c_dev)) {
		ret = PTR_ERR(i2c_dev);
		return ret;
	}

	dev = device_create(i2c_dev_class, &client->adapter->dev, MKDEV(I2C_MAJOR,
					client->adapter->nr), NULL, "pixcir_i2c_ts%d", 0);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		return ret;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	tsdata->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	tsdata->early_suspend.suspend = pixcir_ts_early_suspend;
	tsdata->early_suspend.resume = pixcir_ts_late_resume;
	register_early_suspend(&tsdata->early_suspend);
#endif

	//ctp_irq_enable(tsdata);

	dev_err(&tsdata->client->dev, "insmod successfully!\n");
	
	return 0;

err_free_irq:
	free_irq(client->irq, tsdata);
err_free_mem:
	input_free_device(input);
	kfree(tsdata);
	
	return ret;
}

static int __devexit pixcir_ts_remove(struct i2c_client *client)
{
	int error;
	struct i2c_dev *i2c_dev;
	struct pixcir_i2c_ts_data *tsdata = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&tsdata->early_suspend);
#endif

	//device_init_wakeup(&client->dev, 0);
	cancel_work_sync(&tsdata->work);
	ctp_gpio_as_input(GTP_INT_GPIO);
    ctp_gpio_free(GTP_INT_GPIO);
    sw_gpio_irq_free(int_handle);
	
	/*********************************Bee-0928-TOP****************************************/
	i2c_dev = get_free_i2c_dev(client->adapter);
	if (IS_ERR(i2c_dev)) {
		error = PTR_ERR(i2c_dev);
		return error;
	}

	return_i2c_dev(i2c_dev);
	device_destroy(i2c_dev_class, MKDEV(I2C_MAJOR, client->adapter->nr));
	/*********************************Bee-0928-BOTTOM****************************************/

	input_unregister_device(tsdata->input);
	kfree(tsdata);

	return 0;
}

static int pixcir_open(struct inode *inode, struct file *file) {
	int subminor;
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	struct i2c_dev *i2c_dev;
	int ret = 0;

	pr_info("enter pixcir_open function\n");
	subminor = iminor(inode);

	//lock_kernel();
	i2c_dev = i2c_dev_get_by_minor(subminor);
	if (!i2c_dev) {
		printk("error i2c_dev\n");
		return -ENODEV;
	}

	adapter = i2c_get_adapter(i2c_dev->adap->nr);
	if (!adapter) {
		return -ENODEV;
	}

	client = kzalloc(sizeof(*client), GFP_KERNEL);

	if (!client) {
		i2c_put_adapter(adapter);
		ret = -ENOMEM;
	}

	snprintf(client->name, I2C_NAME_SIZE, "pixcir_ts%d", adapter->nr);
	client->driver = &pixcir_i2c_ts_driver;
	client->adapter = adapter;

	file->private_data = client;

	return 0;
}

static long pixcir_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	struct i2c_client *client = (struct i2c_client *) file->private_data;

	pr_info("pixcir_ioctl(),cmd = %d, arg = %ld\n", cmd, arg);

	switch (cmd) {
	case CALIBRATION_FLAG: //CALIBRATION_FLAG = 1
		client->addr = SLAVE_ADDR;
		status_reg = CALIBRATION_FLAG;
		break;

	case BOOTLOADER: //BOOTLOADER = 7
		pr_info("bootloader\n");
		client->addr = BOOTLOADER_ADDR;
		status_reg = BOOTLOADER;

		work_pending = 0;
		bootloader_irq = 0;
		pixcir_reset();
		mdelay(3);
		irq_flag = 1;
		enable_irq(irq_num);
		work_pending = 2;

		break;

	case RESET_TP: //RESET_TP = 9
		pr_info("reset tp\n");
		pixcir_reset();
		work_pending = 1;
		break;

	case ENABLE_IRQ: //ENABLE_IRQ = 10
		status_reg = 0;
		if (irq_flag == 0) {
			irq_flag = 1;
			enable_irq(irq_num);
		}
		break;

	case DISABLE_IRQ: //DISABLE_IRQ = 11
		if (irq_flag == 1) {
			irq_flag = 0;
			disable_irq_nosync(irq_num);
		}
		break;

	case BOOTLOADER_STU: //BOOTLOADER_STU = 16
		client->addr = BOOTLOADER_ADDR;
		status_reg = BOOTLOADER_STU;

		pixcir_reset();
		mdelay(3);
		break;

	case ATTB_VALUE: //ATTB_VALUE = 17
		client->addr = SLAVE_ADDR;
		status_reg = ATTB_VALUE;
		break;

	default:
		client->addr = SLAVE_ADDR;
		status_reg = 0;
		break;
	}
	return 0;
}

static ssize_t pixcir_read (struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	unsigned char *tmp, bootloader_stu[4], attb_value[1];
	int ret = 0;

	switch(status_reg)
	{
		case BOOTLOADER_STU:
		i2c_master_recv(client, bootloader_stu, sizeof(bootloader_stu));
		if (ret!=sizeof(bootloader_stu)) {
			dev_err(&client->dev,
					"%s: BOOTLOADER_STU: i2c_master_recv() failed, ret=%d\n",
					__func__, ret);
			return -EFAULT;
		}

		if (copy_to_user(buf, bootloader_stu, sizeof(bootloader_stu))) {
			dev_err(&client->dev,
					"%s: BOOTLOADER_STU: copy_to_user() failed.\n", __func__);
			return -EFAULT;
		}
		else {
			ret = 4;
		}
		break;

		case ATTB_VALUE:
		attb_value[0] = ctp_get_attb_value();
		if(copy_to_user(buf, attb_value, sizeof(attb_value))) {
			dev_err(&client->dev,
					"%s: ATTB_VALUE: copy_to_user() failed.\n", __func__);
			return -EFAULT;
		}
		else {
			ret = 1;
		}
		break;

		default:
		tmp = kmalloc(count, GFP_KERNEL);
		if (tmp==NULL)
		return -ENOMEM;

		ret = i2c_master_recv(client, tmp, count);
		if (ret != count) {
			dev_err(&client->dev,
					"%s: default: i2c_master_recv() failed, ret=%d\n",
					__func__, ret);
			return -EFAULT;
		}
		pr_info(">>>>>>>>>>%s:default:%d,%d\n",__func__, tmp[0], tmp[1]);
		if(copy_to_user(buf, tmp, count)) {
			dev_err(&client->dev,
					"%s: default: copy_to_user() failed.\n", __func__);
			kfree(tmp);
			return -EFAULT;
		}
		kfree(tmp);
		break;
	}
	return ret;
}

static ssize_t pixcir_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	struct i2c_client *client;
	unsigned char *tmp, bootload_data[143];
	int ret = 0, time_out = 0;
	client = file->private_data;

	switch(status_reg)
	{
		case CALIBRATION_FLAG: //CALIBRATION_FLAG=1
		tmp = kmalloc(count, GFP_KERNEL);
		if (tmp == NULL)
		return -ENOMEM;

		if (copy_from_user(tmp, buf, count)) {
			dev_err(&client->dev,
					"%s: CALIBRATION_FLAG: copy_from_user() failed.\n", __func__);
			kfree(tmp);
			return -EFAULT;
		}

		ret = i2c_master_send(client, tmp, count);
		if (ret != count) {
			dev_err(&client->dev,
					"%s: CALIBRATION: i2c_master_send() failed, ret=%d\n",
					__func__, ret);
			kfree(tmp);
			return -EFAULT;
		}

		kfree(tmp);
		break;

		case BOOTLOADER:
		memset(bootload_data, 0, sizeof(bootload_data));

		if (copy_from_user(bootload_data, buf, count)) {
			dev_err(&client->dev,
					"%s: BOOTLOADER: copy_from_user() failed.\n", __func__);
			return -EFAULT;
		}

		ret = i2c_master_send(client, bootload_data, count);
		if (ret != count) {
			printk("%s----->>>>%x\n", __func__, client->addr);
			dev_err(&client->dev,
					"%s: BOOTLOADER: i2c_master_send() failed, ret = %d\n",
					__func__, ret);
			return -EFAULT;
		}

		time_out = 0;
		while(bootloader_irq==0) {
			if(time_out > bootloader_delay) break;
			else {
				time_out++;
				udelay(BTIME);
			}
		}
		bootloader_irq = 0;
		pr_info("time_out = %d ms\n", time_out/(1000/BTIME));
		break;

		default:
		tmp = kmalloc(count, GFP_KERNEL);
		if (tmp == NULL)
		return -ENOMEM;

		if (copy_from_user(tmp, buf, count)) {
			dev_err(&client->dev,
					"%s: default: copy_from_user() failed.\n", __func__);
			kfree(tmp);
			return -EFAULT;
		}
		pr_info(">>>>>>>>>>%s:default:%d,%d\n",__func__, tmp[0], tmp[1]);
		ret = i2c_master_send(client,tmp,count);
		if (ret != count) {
			dev_err(&client->dev,
					"%s: default: i2c_master_send() failed, ret=%d\n",
					__func__, ret);
			kfree(tmp);
			return -EFAULT;
		}
		kfree(tmp);
		break;
	}
	return ret;
}

static int pixcir_release(struct inode *inode, struct file *file) {
	struct i2c_client *client = file->private_data;

	pr_info("enter pixcir_release funtion\n");
	i2c_put_adapter(client->adapter);
	kfree(client);
	file->private_data = NULL;

	return 0;
}

static const struct file_operations pixcir_i2c_ts_fops = { 
		.owner = THIS_MODULE,
        .open = pixcir_open, 
        .unlocked_ioctl = pixcir_ioctl,
        .read = pixcir_read, 
        .write = pixcir_write, 
        .release = pixcir_release, 
};

static void pixcir_sleep(struct pixcir_i2c_ts_data *tsdata)
{
	unsigned char wrbuf[2] = { 0 };
	int ret;

	pr_info("%s\n", __func__);

	wrbuf[0] = 0x33;
	wrbuf[1] = 0x01; //enter into Sleep mode;
		
	/**************************************************************
	 wrbuf[1]:	0x00: Active mode
	
	 0x01: Sleep mode
	 0xA4: Sleep mode automatically switch
	 0x03: Freeze mode
	 More details see application note 710 power manangement section
	 ****************************************************************/
		 
	ret = i2c_master_send(tsdata->client, wrbuf, 2);
	if (ret != 2) {
		dev_err(&tsdata->client->dev, "%s: i2c_master_send failed(), ret=%d\n",
				__func__, ret);
	}
}

static void pixcir_wakeup(struct pixcir_i2c_ts_data *tsdata)
{
#if 0
	pixcir_reset();
#else
	unsigned char wrbuf[2] = { 0 };
	int ret;

	pr_info("%s\n", __func__);

	wrbuf[0] = 0x33;
	wrbuf[1] = 0;
	ret = i2c_master_send(tsdata->client, wrbuf, 2);
	if (ret != 2) {
		dev_err(&tsdata->client->dev, "%s: i2c_master_send failed(), ret=%d\n",
		        __func__, ret);
	}
#endif
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void pixcir_ts_early_suspend(struct early_suspend *h)
{
	struct pixcir_i2c_ts_data *tsdata;
	tsdata = container_of(h, struct pixcir_i2c_ts_data, early_suspend);
	
	pr_info("%s\n", __func__);
	
	//ctp_irq_disable(tsdata);
	pixcir_sleep(tsdata);
}

static void pixcir_ts_late_resume(struct early_suspend *h)
{
	struct pixcir_i2c_ts_data *tsdata;
	tsdata = container_of(h, struct pixcir_i2c_ts_data, early_suspend);

	pr_info("%s\n", __func__);
	
	pixcir_wakeup(tsdata);
	//ctp_irq_enable(tsdata);
}
#else

#ifdef CONFIG_PM
static int pixcir_ts_suspend(struct i2c_client *client, pm_message_t mesg) 
{
	struct pixcir_i2c_ts_data *tsdata = i2c_get_clientdata(client);

	pr_info("%s\n", __func__);

	//ctp_irq_disable(tsdata);
	pixcir_sleep(tsdata);

	//if (device_may_wakeup(&tsdata->client->dev)) enable_irq_wake(tsdata->irq);

	return 0;
}

static int pixcir_ts_resume(struct i2c_client *client) 
{
	struct pixcir_i2c_ts_data *tsdata = i2c_get_clientdata(client);

	pixcir_wakeup(tsdata);
	//ctp_irq_enable(tsdata);

	//if (device_may_wakeup(&tsdata->client->dev)) disable_irq_wake(tsdata->irq);

	pr_info("%s\n", __func__);

	return 0;
}
#endif

#endif

static const struct i2c_device_id pixcir_i2c_ts_id[] = { 
		{ CTP_NAME, 0 }, 
        { }, 
};
MODULE_DEVICE_TABLE(i2c, pixcir_i2c_ts_id);

static struct i2c_driver pixcir_i2c_ts_driver = {
		.class      = I2C_CLASS_HWMON,
		.probe 		= pixcir_ts_probe, 
		.remove 	= __devexit_p(pixcir_ts_remove), 
		.id_table 	= pixcir_i2c_ts_id, 
		.driver 	= { 
			.owner 	= THIS_MODULE, 
			.name 	= CTP_NAME, 
		},
#ifndef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_PM
		.suspend = pixcir_ts_suspend, 
		.resume = pixcir_ts_resume,
#endif
#endif
		.address_list	= normal_i2c,
};

static int ctp_get_system_config(void)
{  
        twi_id = config_info.twi_id;
        screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;
        if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}

static int __init pixcir_i2c_ts_init(void)
{
	int ret;

	pr_info("CTP driver installing...\n");

	if (input_fetch_sysconfig_para(&(config_info.input_type))) {
		pr_err("%s: ctp_fetch_sysconfig_para err", __func__);
		return 0;
	}
	else {
		ret = input_init_platform_resource(&(config_info.input_type));
		if (0 != ret) {
			pr_err("%s:ctp_ops.init_platform_resource err", __func__);    
		}
	}

	if(config_info.ctp_used == 0){
	    pr_err("*** ctp_used set to 0 !\n");
	    return 0;
	}

	if(!ctp_get_system_config()){
        pr_err("%s:read config fail!\n",__func__);
        return ret;
    }

	pixcir_reset();
	pixcir_i2c_ts_driver.detect = ctp_detect;
	
	pixcir_wq = create_singlethread_workqueue("pixcir_wq");
	if (!pixcir_wq) {
		pr_err("Creat workqueue failed.");
		return -ENOMEM;
	}
	
	ret = register_chrdev(I2C_MAJOR, "pixcir_ts", &pixcir_i2c_ts_fops);
	if (ret) {
		printk(KERN_ERR "%s:register chrdev failed\n", __FILE__);
		return ret;
	}

	i2c_dev_class = class_create(THIS_MODULE, "pixcir_dev");
	if (IS_ERR(i2c_dev_class)) {
		ret = PTR_ERR(i2c_dev_class);
		class_destroy(i2c_dev_class);
	}

	//msleep(100);
	ret = i2c_add_driver(&pixcir_i2c_ts_driver);

	return ret;
}

static void __exit pixcir_i2c_ts_exit(void)
{
	i2c_del_driver(&pixcir_i2c_ts_driver);
	input_free_platform_resource(&(config_info.input_type));
	
	if(pixcir_wq)
		destroy_workqueue(pixcir_wq);
}

module_init(pixcir_i2c_ts_init);
module_exit(pixcir_i2c_ts_exit);

MODULE_AUTHOR("Jianchun Bian <jcbian@pixcir.com.cn>");
MODULE_DESCRIPTION("Pixcir I2C Touchscreen Driver");
MODULE_LICENSE("GPL");
