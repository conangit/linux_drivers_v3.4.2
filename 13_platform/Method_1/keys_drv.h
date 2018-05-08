#ifndef _KEY_DRV_H
#define _KEY_DRV_H

#undef dbg
#define DEBUG_KEYS
#ifdef DEBUG_KEYS
#define dbg(format,...) printk(KERN_DEBUG "[line:%d]" format,  __LINE__, ##__VA_ARGS__)
#else 
#define dbg(format,...) do{}while(0)
#endif

struct key_platform_data {
    const char *name;
    unsigned int index;
};

#endif
