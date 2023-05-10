### Category加载顺序

![image.png](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/1d21d940887e4d38a1658aa4ab374477~tplv-k3u1fbpfcp-watermark.image?)

![image.png](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/f0b1b82283334fb198d96b3d61bc657c~tplv-k3u1fbpfcp-watermark.image?)
解释：`[Student load];`，即走`objc_msgSend`消息机制，先通过`isa`指针找到其类，发现类中没有实现该方法，然后通过`superclass`找到其父类，发现父类有实现，但是分类方法在前面，后面参与编译的在前面，所以最终调用的是分类`Person(Eat)`方法。

compile source按照编译顺序，后编译的会执行，并且分类的优先级比本类高。分类创建的属性，没有成员变量，无法保存住属性值。通过runtime动态将分类的方法合并到类对象、元类对象的方法列表中。class extension (匿名分类\类扩展)在编译期就加入到方法列表中了。Category是运行时加入的

### Category 的实现原理，如何被加载的?
```
struct _category_t {
    const char *name;
    struct _class_t *cls;
    const struct _method_list_t *instance_methods;  // 对象方法列表
    const struct _method_list_t *class_methods;  // 类方法列表
    const struct _protocol_list_t *protocols;  // 协议列表
    const struct _prop_list_t *properties;  // 属性列表
};
```

![image.png](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/cb93346d0d1144539731720f30281df3~tplv-k3u1fbpfcp-watermark.image?)

 category 编译完成的时候和类是分开的，在程序运行时才通过runtime合并在一起。
 _objc_init是Objcet-C runtime的入口函数，主要读取Mach-O文件完成OC的内存布局，以及初始化runtime相关数据结构。这个函数里会调用到两外两个函数，map_images和load_Images
 
 map_images追溯进去发现其内部调用了_read_images函数，_read_images会读取各种类及相关分类的信息。
 
 读取到相关的信息后通过addUnattchedCategoryForClass函数建立类和分类的关联。
 
 建立关联后通过remethodizeClass -> attachCategories重新规划方法列表、协议列表、属性列表，把分类的内容合并到主类
 
 在map_images处理完成后，开始load_images的流程。首先会调用prepare_load_methods做加载准备，这里面会通过schedule_class_load递归查找到NSObject然后从上往下调用类的load方法。
 
 处理完类的load方法后取出非懒加载的分类通过add_category_to_loadable_list添加到一个全局列表里
 
 最后调用call_load_methods调用分类的load函数
 
### 分类中添加实例变量和属性分别会发生什么，还是什么时候会发生问题？
     添加实例变量编译时报错
     添加属性没问题，但是在运行的时候使用这个属性程序crash。原因是没有实例变量也没有set/get方法
     
### 分类中为什么不能添加成员变量（runtime除外）？
      类对象在创建的时候已经定好了成员变量，但是分类是运行时加载的，无法添加。
      类对象里的 class_ro_t 类型的数据在运行期间不能改变，再添加方法和协议都是修改的 class_rw_t 的数据。
      分类添加方法、协议是把category中的方法，协议放在category_t结构体中，再拷贝到类对象里面。但是category_t里面没有成员变量列表。
      虽然category可以写上属性，其实是通过关联对象实现的，需要手动添加setter & getter。    
      
 ### 分类不能添加属性的实质原因

> 1.我们知道在一个类中用@property声明属性，编译器会自动帮我们生成`_成员变量`和`setter/getter`，但分类的指针结构体中，根本没有`成员变量列表`。所以在分类中用`@property`声明属性，既无法生成`_成员变量`也无法生成`setter/getter`的实现。  
> 2.因此结论是：我们可以用@property声明属性，编译和运行都会通过，只要不使用程序也不会崩溃。但如果调用了`_成员变量`和`setter/getter`方法，报错就在所难免了。

### 因为分类会覆盖本类的同名方法，想要调用本类方法怎么做？
 倒序遍历方法列表，找到相同的方法名就行，因为本类的在方法列表第一个
 
 ```Objective-C
void invokeOriginalMethod(id target , SEL selector) {
    uint count;
    Method *list = class_copyMethodList([target class], &count);
    for ( int i = count - 1 ; i >= 0; i--) {
        Method method = list[i];
        SEL name = method_getName(method);
        IMP imp = method_getImplementation(method);
        if (name == selector) {
            ((void (*)(id, SEL))imp)(target, name);
            break;
        }
    }
    free(list);
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        MJPerson *person = [[MJPerson alloc] init];
        invokeOriginalMethod(person, @selector(run));
        
    }
    return 0;
}
 ```

#### +load方法
-   +load方法会在runtime加载类、分类时调用
-   每个类、分类的+load，在程序运行过程中只调用一次
-   `调用顺序`
    -   1.先调用类的+load
        -   按照编译先后顺序调用（先编译，先调用）
        -   调用子类的+load之前会先调用父类的+load

    -   2.再调用分类的+load

        -   按照编译先后顺序调用（先编译，先调用）

-   代码例子及图解佐证

![image.png](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/426dfee073f0437499afdbc917aa72d1~tplv-k3u1fbpfcp-watermark.image?)

##### objc4源码解读过程：objc-os.mm
-   _objc_init
-   load_images
-   prepare_load_methods
    -   schedule_class_load
    -   add_class_to_loadable_list
    -   add_category_to_loadable_list
-   call_load_methods
    -   call_class_loads
    -   call_category_loads
    -   (*load_method)(cls, SEL_load)
-   `+load方法是根据方法地址直接调用，并不是经过objc_msgSend函数调用`

##### + initialize方法
-   `+initialize方法会在类第一次接收到消息时调用`
-   调用顺序
    -   先调用父类的+initialize，再调用子类的+initialize
    -   (先初始化父类，再初始化子类，每个类只会初始化1次)
##### objc4源码解读过程
-   `objc-msg-arm64.s`
    -   objc_msgSend
-   `objc-runtime-new.mm`
    -   class_getInstanceMethod
    -   lookUpImpOrNil
    -   lookUpImpOrForward
    -   _class_initialize
    -   callInitialize
    -   objc_msgSend(cls, SEL_initialize)

项目例子佐证 -

![image.png](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/b73f7793caf54581bf6a2f4eb8e431be~tplv-k3u1fbpfcp-watermark.image?)
-   `每个类都实现了+ initialize方法`

![image.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/390ae24151834a5e959f665b22bb9c6b~tplv-k3u1fbpfcp-watermark.image?)
-   `先调用父类的+initialize，再调用子类的+initialize`
-   `(先初始化父类，再初始化子类，每个类只会初始化1次)`

> 解析  
> 1.[Student alloc]会调用+initialize方法，因为他有父类Person，所以先调用Person的+initialize方法，又因为分类在前面，所以调用了Person(Test2)的+initialize方法。但是他自己本身没有实现+initialize方法，所以会去父类查找，然后分类方法在前面，所以调用了Person(Test2)的+initialize方法。  

> 2.[Teacher alloc]会调用+initialize方法，因为他有父类Person，所以先调用Person的+initialize方法，但是前面已经初始化过了，所以跳过，调用自己的+initialize方法，但是因为他自己没有实现+initialize方法，所以调用父类的+initialize方法，又因为分类方法在前面，所以调用Person(Test) +initialize方法。 

> 3.[Person alloc]，因为前面已经初始化过了，所以不会再调+initialize方法，所以这里不打印。

##### +initialize和+load的区别
-   +initialize是通过objc_msgSend进行调用的，所以有以下特点
    -   如果子类没有实现+initialize，会调用父类的+initialize（所以父类的+initialize可能会被调用多次）
    -   如果分类实现了+initialize，就覆盖类本身的+initialize调用

### 关联对象
    使用关联对象，需要在主对象 dealloc 的时候手动释放么？
     不需要，主对象通过 dealloc -> object_dispose -> object_remove_assocations 进行关联对象的释放
     
```Objective-C     
#import "MJPerson+Test.h"
#import <objc/runtime.h>

@implementation MJPerson (Test)
/**
 void objc_setAssociatedObject(id _Nonnull object, const void * _Nonnull key,
                         id _Nullable value, objc_AssociationPolicy policy)
 AssociationsManager = {static AssociationsHashMap map}
 AssociationsHashMap map = [{object : AssociationsMap = {key : Associations = {policy,value}}}]
 */
- (void)setName:(NSString *)name  //key确保唯一就行，_cmd、@selector(name)、static类型的字符串都可以作为key
{
    objc_setAssociatedObject(self, @selector(name), name, OBJC_ASSOCIATION_COPY_NONATOMIC);
}

- (NSString *)name
{
    // 隐式参数
    // _cmd == @selector(name)
    return objc_getAssociatedObject(self, _cmd);
}

- (void)setWeight:(int)weight
{
    objc_setAssociatedObject(self, @selector(weight), @(weight), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (int)weight
{
    // _cmd == @selector(weight)
    return [objc_getAssociatedObject(self, _cmd) intValue];
}

@end
```
     
关联对象的实现和原理
 关联对象不存储在关联对象本身内存中，而是存储在一个全局容器中；
 这个容器是由 AssociationsManager 管理并在它维护的一个单例 Hash 表AssociationsHashMap ；

 第一层 AssociationsHashMap：类名object ：bucket（map）
 第二层 ObjectAssociationMap：key（name）：ObjcAssociation（value和policy）
 
 AssociationsManager 使用 AssociationsManagerLock 自旋锁保证了线程安全。
 通过objc_setAssociatedObject给某对象添加关联值
 通过objc_getAssociatedObject获取某对象的关联值
 通过objc_removeAssociatedObjects移除某对象的关联值 

![图片](/图片/关联对象结构.png)