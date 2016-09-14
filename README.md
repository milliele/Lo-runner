Loco program runner core
========================


We use this python-c library to run program in a sandbox-like environment.
With it, we can accurately known the resource using of the program and 
limit its resource using including system-call interrupt.

我们可以使用这个Python-C编译成的库来将一个程序运行在一个类似沙箱的环境中。
我们可以正确的得知这个程序运行所占用的资源，以及使用系统中断来限制程序的资源使用。

Usage（使用）
-----

For run a program without tracing:

如果使用trace模式，示例代码如下：

    runcfg = {
        'args':['./m'],
        'fd_in':fin.fileno(),
        'fd_out':ftemp.fileno(),
        'timelimit':1000, #in MS
        'memorylimit':20000, #in KB
    }
    
    rst = lorun.run(runcfg)

For check one output:
检查其中一个测试数据的运行结果：

    ftemp = file('temp.out')
    fout = file(out_path)
    crst = lorun.check(fout.fileno(), ftemp.fileno())


Trace（跟踪模式）
-----

You can set runcfg['trace'] to True to ensure runner's security.

你可以通过设置 runcfg['trace'] 为 True 来确保Loruner运行在安全的环境中

Here is a simple usage:

示例代码：

```
runcfg['trace'] = True
"""
正常可以使用range(0, 400)
可以选择性屏蔽一些关键函数的使用：
具体的值可以参考：http://www.cnblogs.com/jiangzhaowei/p/4192290.html
"""
runcfg['calls'] = range(0, 400) 
""" 
值非0则是可以访问，64位linux请加上 /lib/x86_64-linux-gnu/libc.so.6
"""
runcfg['files'] = {'/etc/ld.so.cache': 1} 

```

WeJudge
-----
http://oj.bnuz.edu.cn/

感谢这个开源的Lo-runner项目，让我可以开发出一个基于Python+Django框架的OJ系统。这个系统开发历时一年，主要用于学校的C语言教学以及学校的ACM比赛使用，是由我个人独立完成的一套完整的在线评测系统。

这个判题机核心代码是非常好的学习资源，目前我也对其进行了一些小小的改动，以提高计时器精度和内存限制精度。

感谢作者的源码共享，基于作者的GNU协议，现在将代码继续开源共享，并将在近期对OJ系统使用GNU协议继续进行开源。

—— LanceLRQ 北京师范大学珠海分校
