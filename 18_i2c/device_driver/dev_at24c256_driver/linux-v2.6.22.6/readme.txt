怎么写I2C设备驱动
1.分配一个i2c_driver结构体
2.设置
    attach_adapter  //调用i2c_probe(&adapter, &address_data, int (*found_proc) (struct i2c_adapter *, int, int))
    detach_client   //卸载这个驱动后 如果之前发现能够支持这个设备 则调用它来清理
3.注册/卸载
i2c_add_driver()
i2c_del_driver()

