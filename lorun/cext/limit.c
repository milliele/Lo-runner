/**
 * Loco program runner core
 * Copyright (C) 2011  Lodevil(Du Jiong)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "limit.h"
#include <sys/resource.h>
#include <sys/time.h>

const char *last_limit_err;

int setResLimit(struct Runobj *runobj) {
#define RAISE_EXIT(err) {last_limit_err = err;return -1;}
    struct rlimit rl;
    struct itimerval p_realt;
    /*
        事实上，这段代码的使用，只能让CPU时间精确到单位秒，而且根据实际情况的调试发现，
        rl.rlim_cur = runobj->time_limit / 1000 + 1;
        if (runobj->time_limit % 1000 > 800) {
            rl.rlim_cur += 1;
        }
        rl.rlim_max = rl.rlim_cur + 1;
        if (setrlimit(RLIMIT_CPU, &rl))
            RAISE_EXIT("set RLIMIT_CPU failure");
        这段代码能够使得控制的运行时间比预设时间多出整整一秒（也不知道为啥作者要这样写）。
        rl.rlim_cur = runobj->time_limit / 1000;
    		if(runobj->time_limit % 1000 > 0){
    				rl.rlim_cur += 1;
    		}
        将其中一部分改成这样，单独运行判题机核心逻辑，时间控制在990ms~1000ms，
        但在实际工作环境上，在800ms~1200ms之间浮动，原因未知。

        原作者使用这段代码的用意是准确的判定程序所用CPU的时间而不是运行时间。
        不过，在精度上，仅能精确到秒。超时捕获到SIGXCPU(9)的信号。

        文档请参考：http://linux.die.net/man/2/setrlimit
    */
    rl.rlim_cur = runobj->time_limit / 1000;
    if(runobj->time_limit % 1000 > 0){
        rl.rlim_cur += 1;
    }
    rl.rlim_max = rl.rlim_cur + 1;
    if (setrlimit(RLIMIT_CPU, &rl))
        RAISE_EXIT("set RLIMIT_CPU failure");
    /*
    感谢作者没有删除这段被注释的代码，才让我能够将时间控制在毫秒的级别
    线上环境测试所得，取消注释后的代码依旧没有改进，于是查询文档后得知
    it_interval 和 it_interval参数必须同时设置
    tv_sec单位为秒，tv_usec单位为微秒(10^-6)
    而我们设置的runobj->time_limit单位通常为毫秒，并且，tv的两个参数是联合描述的：
    也就是说，我们要告诉setitimer函数是tv_sec秒tv_usec微秒的时候给出一个SIGALRM(14)信号
    定时器的使用计算的是程序实际运行的时间，在得到进度的同时，也不可避免的可能会影响题目程序预期CPU时间
    这是为了精度的妥协（怪我咯，谁让CPU时间限制单位是秒，ACM协会说要毫秒精度，我也没办法...）
    */
    p_realt.it_interval.tv_sec = runobj->time_limit / 1000;
    p_realt.it_interval.tv_usec = (runobj->time_limit % 1000) * 1000;
    p_realt.it_value = p_realt.it_interval;
    if (setitimer(ITIMER_VIRTUAL, &p_realt, NULL) == -1)
        RAISE_EXIT("set ITIMER_VIRTUAL failure");

    rl.rlim_cur = runobj->memory_limit * 1024 + 1024 * 1024; //附加1MB内存修正
    rl.rlim_max = rl.rlim_cur + 1024;
    if (setrlimit(RLIMIT_DATA, &rl))
        RAISE_EXIT("set RLIMIT_DATA failure");

    rl.rlim_cur = runobj->memory_limit * 1024 + 3 * 1024 * 1024; //附加3MB内存修正
    rl.rlim_max = rl.rlim_cur + 1024;
    if (setrlimit(RLIMIT_AS, &rl))
        RAISE_EXIT("set RLIMIT_AS failure");

    // 栈空间设置限制为8MB
    rl.rlim_cur = 8 * 1024 * 1024;
    rl.rlim_max = rl.rlim_cur + 1024;
    if (setrlimit(RLIMIT_STACK, &rl))
        RAISE_EXIT("set RLIMIT_STACK failure");

    // 文件读写限制：设想下，如果运行一个死循环，输出Hello World，会生成将近1个G的文件
    // 原作者并没有想到这个问题。。。可能因为比较土豪吧。
    rl.rlim_cur = 100 * 1024 * 1024;
    rl.rlim_max = rl.rlim_cur;
    if (setrlimit(RLIMIT_FSIZE, &rl))
        RAISE_EXIT("set RLIMIT_FSIZE failure");

    return 0;
}
