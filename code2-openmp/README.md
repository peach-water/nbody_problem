# openmp 
openmp库的学习使用。
实现一个并行化的计数排序
# 环境要求
omp.h，也推荐在linux系统中使用。Windows印象中好像也有这个库文件。
怎么添加这个库文件那就是另外需要自己去解决的问题了。
# 代码解释
这是我学到的最偷懒的并行化代码了。把串行的代码添加如下一行就变成并行化的代码了，比如这一行需要贴在for循环代码前
```C++
#pragma omp parallel for num_threads(thread_count) private(tid,i, j, count) shared(n, a, temp)
```
parallel for表示这里并行化的是for循环，num_threads(thread_cound)表示用thread_count个线程并行，private(tid, i, j, count)表示这tid，i，j，count是私有化变量不同线程之间不会互相干扰，对应的share是共享变量。

去掉for，还可以并行化特定的代码块，这一行代码只影响紧接着的代码或者代码块。
