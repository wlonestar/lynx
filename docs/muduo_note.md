# muduo 笔记

## 线程安全的对象生命期管理

### 1. 线程安全的对象构造：在构造期间不要泄漏 this 指针

- 不要在构造函数中注册任何回调
- 不要在构造函数中把 this 传给跨线程的对象
- 在构造函数最后一行也不行

```cpp
class Foo : public Observer {
public:
  Foo();
  virtual void update();
  virtual observe(Observable *s) {
    s->register_(this);
  }
};

Foo *pFoo = new Foo;
Observable *s = getSubject();
pFoo->observe(s); // or s->register_(pFoo);
```

二段式构造（构造函数+initialize()）有时会是好办法。

### 2. 智能指针

- shared_ptr 控制对象的生命期，是强引用，只要有一个指向 x 对象的 shared_ptr 存在，该 x 对象就不会析构。
- weak_ptr 不控制对象的生命期，但知道对象是否存活。如果对象还存活，可以提升为有效的 shared_ptr；如果对象已经死了，提升会失败，返回空的 shared_ptr。

### 3. 小结

- 原始指针暴露给多个线程往往会造成 race condition 或额外的簿记负担。
- 统一用 shared_ptr//scoped_ptr 来管理对象的生命期，在多线程中尤其重要。
- shared_ptr 是值语意，当心意外延长对象的生命期。例如 std::bind 和容器都可能拷贝 shared_ptr。
- weak_ptr 是 shared_ptr 的好搭档，可以用作弱回调、对象池等。
- 认真阅读一遍 boost::shared_ptr 的文档，能学到很多东西：
  http://www.boost.org/doc/libs/release/libs/smart_ptr/shared_ptr.htm
- 保持开放心态，留意更好的解决办法，比如 C++11 引入的 unique_ptr。忘掉已被废弃的 auto_ptr。

## 线程同步精要

### 1. 只用非递归的互斥器

### 2. 不要用读写锁和信号量

- 读写锁不见得比普通 mutex 更高效
- 信号量有自己的计数值，而通常我们的数据结构也有长度值，这造成了同样的信息存了两份，，需要时刻保持一致。

### 3. 线程安全的 Singleton 实现：pthread_once
