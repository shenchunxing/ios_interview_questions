//
//  main.m
//  Interview02-runtime应用
//
//  Created by MJ Lee on 2018/5/29.
//  Copyright © 2018年 MJ Lee. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MJPerson.h"
#import "MJStudent.h"
#import "MJCar.h"
#import <objc/runtime.h>
#import "NSObject+Json.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // 字典转模型
        NSDictionary *json = @{
                               @"id" : @20,
                               @"age" : @20,
                               @"weight" : @60,
                               @"name" : @"Jack"
//                               @"no" : @30
                               };
        
        MJPerson *person = [MJPerson mj_objectWithJson:json];
        
//        [MJCar mj_objectWithJson:json];
        
//        MJStudent *student = [MJStudent mj_objectWithJson:json];
        
        //已经存在的类不能添加成员变量,因为内存结构已经确定
//        class_addIvar([MJPerson class], "_width", 4, 1, @encode(int));
//        [person valueForKey:@"_width"];
        
        NSLog(@"123");
    }
    return 0;
}

void testIvars()
{
    // 获取成员变量信息
    //        Ivar ageIvar = class_getInstanceVariable([MJPerson class], "_age");
    //        NSLog(@"%s %s", ivar_getName(ageIvar), ivar_getTypeEncoding(ageIvar));
    
    // 设置和获取成员变量的值
    //        Ivar nameIvar = class_getInstanceVariable([MJPerson class], "_name");
    //
    //        MJPerson *person = [[MJPerson alloc] init];
    //        object_setIvar(person, nameIvar, @"123");
    //        object_setIvar(person, ageIvar, (__bridge id)(void *)10);
    //        NSLog(@"%@ %d", person.name, person.age);
    
    // 成员变量的数量
    unsigned int count;
    Ivar *ivars = class_copyIvarList([MJPerson class], &count);
    for (int i = 0; i < count; i++) {
        // 取出i位置的成员变量
        Ivar ivar = ivars[i];
        NSLog(@"%s %s", ivar_getName(ivar), ivar_getTypeEncoding(ivar));
    }
    free(ivars);
}

void run(id self, SEL _cmd)
{
    NSLog(@"_____ %@ - %@", self, NSStringFromSelector(_cmd));
}

void testClass()
{
    // 创建类
    Class newClass = objc_allocateClassPair([NSObject class], "MJDog", 0);
    class_addIvar(newClass, "_age", 4, 1, @encode(int));
    class_addIvar(newClass, "_weight", 4, 1, @encode(int));
    class_addMethod(newClass, @selector(run), (IMP)run, "v@:");
    // 注册类
    objc_registerClassPair(newClass);
    
    //        MJPerson *person = [[MJPerson alloc] init];
    //        object_setClass(person, newClass);
    //        [person run];
    
    //        id dog = [[newClass alloc] init];
    //        [dog setValue:@10 forKey:@"_age"];
    //        [dog setValue:@20 forKey:@"_weight"];
    //        [dog run];
    //
    //        NSLog(@"%@ %@", [dog valueForKey:@"_age"], [dog valueForKey:@"_weight"]);
    
    // 在不需要这个类时释放
    objc_disposeClassPair(newClass);
}

void test()
{
    MJPerson *person = [[MJPerson alloc] init];
    [person run];
    
    object_setClass(person, [MJCar class]);
    [person run];
    
    NSLog(@"%d %d %d",
          object_isClass(person),
          object_isClass([MJPerson class]),
          object_isClass(object_getClass([MJPerson class]))
          );
    
    //        NSLog(@"%p %p", object_getClass([MJPerson class]), [MJPerson class]);
}
