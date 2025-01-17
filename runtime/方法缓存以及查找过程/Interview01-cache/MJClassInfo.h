//
//  MJClassInfo.h
//  TestClass
//
//  Created by MJ Lee on 2018/3/8.
//  Copyright © 2018年 MJ Lee. All rights reserved.
//

#import <Foundation/Foundation.h>

#ifndef MJClassInfo_h
#define MJClassInfo_h

# if __arm64__
#   define ISA_MASK        0x0000000ffffffff8ULL
# elif __x86_64__
#   define ISA_MASK        0x00007ffffffffff8ULL
# endif

#if __LP64__
typedef uint32_t mask_t;
#else
typedef uint16_t mask_t;
#endif
typedef uintptr_t cache_key_t;

#if __arm__  ||  __x86_64__  ||  __i386__
// objc_msgSend has few registers available.
// Cache scan increments and wraps at special end-marking bucket.
#define CACHE_END_MARKER 1
static inline mask_t cache_next(mask_t i, mask_t mask) {
    return (i+1) & mask;
}

#elif __arm64__
// objc_msgSend has lots of registers available.
// Cache scan decrements. No end marker needed.
#define CACHE_END_MARKER 0
static inline mask_t cache_next(mask_t i, mask_t mask) {
    return i ? i-1 : mask;
}

#else
#error unknown architecture
#endif

struct bucket_t {//桶
    cache_key_t _key;//方法key
    IMP _imp;//方法实现
};

struct cache_t { //方法缓存
    bucket_t *_buckets;//桶数组
    mask_t _mask;//掩码
    mask_t _occupied;//方法数量-1
    
    IMP imp(SEL selector)//根据Sel查找方法实现
    {
        mask_t begin = _mask & (long long)selector;
        mask_t i = begin;
        do {
            if (_buckets[i]._key == 0  ||  _buckets[i]._key == (long long)selector) {
                return _buckets[i]._imp;
            }
        } while ((i = cache_next(i, _mask)) != begin);
        return NULL;
    }
};

struct entsize_list_tt {
    uint32_t entsizeAndFlags;
    uint32_t count;
};

struct method_t { //方法
    SEL name;//方法名
    const char *types;//方法类型
    IMP imp;//方法实现
};

struct method_list_t : entsize_list_tt {
    method_t first;
};

struct ivar_t {//成员
    int32_t *offset;//偏移量
    const char *name;//名称
    const char *type;//类型
    uint32_t alignment_raw;//对齐
    uint32_t size;//大小
};

struct ivar_list_t : entsize_list_tt {
    ivar_t first;
};

struct property_t {
    const char *name;//属性名称
    const char *attributes;//属性信息
};

struct property_list_t : entsize_list_tt {
    property_t first;
};

struct chained_property_list {
    chained_property_list *next;
    uint32_t count;
    property_t list[0];
};

typedef uintptr_t protocol_ref_t;
struct protocol_list_t {
    uintptr_t count;
    protocol_ref_t list[0];
};

struct class_ro_t {//只读
    uint32_t flags;
    uint32_t instanceStart;
    uint32_t instanceSize;  // instance对象占用的内存空间
#ifdef __LP64__
    uint32_t reserved;
#endif
    const uint8_t * ivarLayout;
    const char * name;  // 类名
    method_list_t * baseMethodList;//方法列表
    protocol_list_t * baseProtocols;//协议列表
    const ivar_list_t * ivars;  // 成员变量列表
    const uint8_t * weakIvarLayout;
    property_list_t *baseProperties;//属性列表
};

struct class_rw_t {
    uint32_t flags;
    uint32_t version;
    const class_ro_t *ro;//只读
    method_list_t * methods;    // 方法列表
    property_list_t *properties;    // 属性列表
    const protocol_list_t * protocols;  // 协议列表
    Class firstSubclass;
    Class nextSiblingClass;
    char *demangledName;
};

#define FAST_DATA_MASK          0x00007ffffffffff8UL
struct class_data_bits_t {
    uintptr_t bits;
public:
    class_rw_t* data() {
        return (class_rw_t *)(bits & FAST_DATA_MASK);
    }
};

/* OC对象 */
struct mj_objc_object {
    void *isa;
};

/* 类对象 */
struct mj_objc_class : mj_objc_object {
    Class superclass;//父类
    cache_t cache;//方法缓存
    class_data_bits_t bits;//信息
public:
    class_rw_t* data() {
        return bits.data();
    }
    
    mj_objc_class* metaClass() {//元类
        return (mj_objc_class *)((long long)isa & ISA_MASK);
    }
};

#endif /* MJClassInfo_h */
