[Llinux-2.6.22]

i2c_add_driver(&i2cdev_driver)

    struct i2cdev_driver {
        .attach_adapter	= i2cdev_attach_adapter, --> device_create()
        .detach_adapter	= i2cdev_detach_adapter, --> device_destroy()
    }


[Llinux-3.4.2]

bus_register_notifier(&i2c_bus_type, &i2cdev_notifier)
    
    static struct notifier_block i2cdev_notifier = {
        .notifier_call = i2cdev_notifier_call,
    };
    
        i2cdev_notifier_call()
            
            BUS_NOTIFY_ADD_DEVICE
                i2cdev_attach_adapter --> device_create()
            BUS_NOTIFY_DEL_DEVICE
                i2cdev_detach_adapter --> device_destroy()