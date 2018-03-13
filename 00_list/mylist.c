#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>

struct list_head mylist;

typedef struct STUDENT {
    char name[16];
    int age;
    int score;
    struct list_head header;
}st;

st s1 = {.name = "lihong", .age = 27, .score = 60};
st s2 = {.name = "hanmei", .age = 25, .score = 85};
st s3 = {.name = "liuran", .age = 35, .score = 76};

static int __init mylist_init(void)
{
    struct list_head *pos;
    st *pv;

    INIT_LIST_HEAD(&mylist);

    list_add_tail(&(s1.header), &mylist);
    list_add_tail(&(s2.header), &mylist);
    list_add_tail(&(s3.header), &mylist);

    list_for_each(pos, &mylist)
    {
        pv = (st *)list_entry(pos, st, header);
        printk("name: %s, age: %d, score: %d\n", pv->name, pv->age, pv->score);
    }

    return 0;
}

static void __exit mylist_exit(void)
{
    list_del(&(s3.header));
    list_del(&(s2.header));
    list_del(&(s1.header));
}

module_init(mylist_init);
module_exit(mylist_exit);

MODULE_DESCRIPTION("kernel list demo");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihong");
MODULE_VERSION("v1.0.0");



