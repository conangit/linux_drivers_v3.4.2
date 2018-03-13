#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>

struct test {
    char *name;
    int a;
    int b;
};


int app(void)
{
    char *p = "lihong";
    struct test t = {.name = p, .a = 10, .b = 20,};
    int *pi;

    printk(KERN_DEBUG "init: name = %s, a = %d, b = %d\n", t.name, t.a, t.b);

    pi = &t.a;

    struct test *ptr = container_of(pi, struct test, a);    //container_of(ptr, type, member)
    //struct test *ptr = container_of(int *, struct test, a); //大错特错

    printk(KERN_DEBUG "_of: name = %s, a = %d, b = %d\n", ptr->name, ptr->a, ptr->b);

    return 0;
}



int __init test_init(void)
{
    app();
    
    return 0;
}


void __exit test_exit(void)
{


}



module_init(test_init);
module_exit(test_exit);

MODULE_DESCRIPTION("test driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihong");
MODULE_VERSION("v1.0.0");


