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

#include "run.h"
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "access.h"
#include "limit.h"
#include <stdio.h>

const char *last_run_err;
#define RAISE_RUN(err) {last_run_err = err;return -1;}

int traceLoop(struct Runobj *runobj, struct Result *rst, pid_t pid) {
    int status, incall = 0;
    struct rusage ru;
    struct user_regs_struct regs;

    while (1) {
        if (wait4(pid, &status, WSTOPPED, &ru) == -1)
            RAISE_RUN("wait4 [WSTOPPED] failure");

        if (WIFEXITED(status))
            break;
        else if (WSTOPSIG(status) != SIGTRAP) {
            ptrace(PTRACE_KILL, pid, NULL, NULL);
            waitpid(pid, NULL, 0);

            rst->time_used = ru.ru_utime.tv_sec * 1000
                    + ru.ru_utime.tv_usec / 1000
                    + ru.ru_stime.tv_sec * 1000
                    + ru.ru_stime.tv_usec / 1000;
            rst->memory_used = ru.ru_maxrss;

            switch (WSTOPSIG(status)) {
                case SIGSEGV:
                    if (rst->memory_used > runobj->memory_limit)
                        rst->judge_result = MLE;
                    else
                        rst->judge_result = RE;
                    break;
                case SIGALRM:
                case SIGVTALRM:
                case SIGXCPU:
                    rst->judge_result = TLE;
                    break;
                default:
                    rst->judge_result = RE;
                    break;
            }

            rst->re_signum = WSTOPSIG(status);
            rst->time_used = ru.ru_utime.tv_sec * 1000
                    + ru.ru_utime.tv_usec / 1000
                    + ru.ru_stime.tv_sec * 1000
                    + ru.ru_stime.tv_usec / 1000;
            rst->memory_used = ru.ru_maxrss;
            return 0;
        }

        if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1)
            RAISE_RUN("PTRACE_GETREGS failure");

        if (incall) {
            int ret = checkAccess(runobj, pid, &regs);
            if (ret != ACCESS_OK) {
                ptrace(PTRACE_KILL, pid, NULL, NULL);
                waitpid(pid, NULL, 0);

                rst->time_used = ru.ru_utime.tv_sec * 1000
                        + ru.ru_utime.tv_usec / 1000
                        + ru.ru_stime.tv_sec * 1000
                        + ru.ru_stime.tv_usec / 1000;
                rst->memory_used = ru.ru_maxrss
                        * (sysconf(_SC_PAGESIZE) / 1024);

                rst->judge_result = RE;
                if (ret == ACCESS_CALL_ERR) {
                    rst->re_call = REG_SYS_CALL(&regs);
                } else {
                    rst->re_file = lastFileAccess();
                    rst->re_file_flag = REG_ARG_2(&regs);
                }
                return 0;
            }
            incall = 0;
        } else
            incall = 1;

        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    }


    rst->time_used = ru.ru_utime.tv_sec * 1000
            + ru.ru_utime.tv_usec / 1000
            + ru.ru_stime.tv_sec * 1000
            + ru.ru_stime.tv_usec / 1000;
    rst->memory_used = ru.ru_maxrss;


    if (rst->time_used > runobj->time_limit)
        rst->judge_result = TLE;
    else if (rst->memory_used > runobj->memory_limit)
        rst->judge_result = MLE;
    else
        rst->judge_result = AC;

    return 0;
}

int get_memory_usage(pid_t pid) {
    int fd, data;
    char buf[4096], status_child[NAME_MAX];
    char *vm;

    sprintf(status_child, "/proc/%d/status", pid);
    if ((fd = open(status_child, O_RDONLY)) < 0)
        return -1;

    read(fd, buf, 4095);
    buf[4095] = '\0';
    close(fd);

    data = 0;

    vm = strstr(buf, "VmPeak:");
    if (vm) {
        sscanf(vm, "%*s %d", &data);
    }

    return data;    
}

int waitExit(struct Runobj *runobj, struct Result *rst, pid_t pid) {
    int status, incall = 0;
    struct rusage ru;
    struct user_regs_struct regs;

    while (1) {
        if (wait4(pid, &status, WSTOPPED, &ru) == -1)
            RAISE_RUN("wait4 [WSTOPPED] failure");

        if (WIFEXITED(status)) {
            break;
        }
        else if (WSTOPSIG(status) != SIGTRAP) {
            rst->memory_used = get_memory_usage(pid);

            ptrace(PTRACE_KILL, pid, NULL, NULL);
            waitpid(pid, NULL, 0);

            rst->time_used = ru.ru_utime.tv_sec * 1000
                    + ru.ru_utime.tv_usec / 1000
                    + ru.ru_stime.tv_sec * 1000
                    + ru.ru_stime.tv_usec / 1000;
            

            switch (WSTOPSIG(status)) {
                case SIGSEGV:
                    //如果是超过了内存限制，那么子进程为自己分配内存空间会失败，后面访问这些空间会segment fault
                    //所以SIGSEGV有可能是RE，有可能是MLE
                    if (rst->memory_used > runobj->memory_limit)
                        rst->judge_result = MLE;
                    else
                        rst->judge_result = RE;
                    break;
                case SIGALRM:
                case SIGVTALRM:
                case SIGXCPU:
                    //要么超过CPU时间限制，要么超过真实时间限制，都是TLE
                    rst->judge_result = TLE;
                    break;
                default:
                    //其它的非正常abort，就是RE
                    rst->judge_result = RE;
                    break;
            }

            return 0;
        }

        //获取系统调用号
        if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1)
            RAISE_RUN("PTRACE_GETREGS failure");

        if (incall) {
            unsigned long syscall_number = REG_SYS_CALL(&regs);
            // printf("%ld\n", syscall_number);
            if (syscall_number == SYS_exit || syscall_number == SYS_exit_group) {
                // 拦截exit调用，读取内存使用
                rst->memory_used = get_memory_usage(pid);
            }
            incall = 0;
        } else
            incall = 1;
        //继续系统调用的默认操作
        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    }


    rst->time_used = ru.ru_utime.tv_sec * 1000
            + ru.ru_utime.tv_usec / 1000
            + ru.ru_stime.tv_sec * 1000
            + ru.ru_stime.tv_usec / 1000;

    if (rst->time_used > runobj->time_limit)
        rst->judge_result = TLE;
    else if (rst->memory_used > runobj->memory_limit)
        rst->judge_result = MLE;
    else
        rst->judge_result = AC;

    return 0;



    // int status;
    // struct rusage ru;

    // if (wait4(pid, &status, WUNTRACED, &ru) == -1)
    //     RAISE_RUN("wait4 failure");

    // rst->time_used = ru.ru_utime.tv_sec * 1000
    //         + ru.ru_utime.tv_usec / 1000
    //         + ru.ru_stime.tv_sec * 1000
    //         + ru.ru_stime.tv_usec / 1000;    
    
    // rst->memory_used = ru.ru_maxrss;
    
    // if (WIFSIGNALED(status)) {
    //     switch (WTERMSIG(status)) {
    //         case SIGSEGV:
    //             if (rst->memory_used > runobj->memory_limit)
    //                  rst->judge_result = MLE;
    //             else
    //                  rst->judge_result = RE;
    //             /*
    //             使用 setrlimt 设置 RLIMIT_AS 这个限制的时候，从Linux官方文档可以得知，一定会引发SIGSEGV信号。
    //             */
    //             break;
    //         case SIGXFSZ:
    //             rst->judge_result = OLE;
    //             break;
    //         case SIGALRM:
    //         case SIGVTALRM:
    //         case SIGXCPU:
    //         case SIGPROF:
    //             rst->judge_result = TLE;
    //             break;
    //         case SIGKILL:
    //             // 我并不知道我为什么要加入这个东西，以前改进去的
    //             // 大概就是说，如果太过分MLE会触发系统的 SIGKILL信号强制退出
    //             // 所以呢，如果是因为用时超过限制的话，其实是可以判断的，否则就是MLE了。
    //             if(rst->time_used > (runobj->time_limit - 100)){
    //                 rst->judge_result = TLE;
    //             }else{
    //                 rst->judge_result = MLE;
    //             }
    //             break;
    //         default:
    //             rst->judge_result = RE;
    //             break;
    //     }
    //     rst->re_signum = WTERMSIG(status);
    // } else {
    //     // if (rst->time_used > runobj->time_limit)
    //     //     rst->judge_result = TLE;
    //     // else if (rst->memory_used > runobj->memory_limit)
    //     //     rst->judge_result = MLE;
    //     // else
    //         rst->judge_result = AC;
    // }

    return 0;
}

int runit(struct Runobj *runobj, struct Result *rst) {
    pid_t pid;
    int fd_err[2];

    if (pipe2(fd_err, O_NONBLOCK))
        RAISE1("run :pipe2(fd_err) failure");

    pid = vfork();
    if (pid < 0) {
        close(fd_err[0]);
        close(fd_err[1]);
        RAISE1("run : vfork failure");
    }

    if (pid == 0) {
        close(fd_err[0]);

#define RAISE_EXIT(err) {\
            int r = write(fd_err[1],err,strlen(err));\
            _exit(r);\
        }

        if (runobj->fd_in != -1)
            if (dup2(runobj->fd_in, 0) == -1)
                RAISE_EXIT("dup2 stdin failure!")

        if (runobj->fd_out != -1)
            if (dup2(runobj->fd_out, 1) == -1)
                RAISE_EXIT("dup2 stdout failure")

        if (runobj->fd_err != -1)
            if (dup2(runobj->fd_err, 2) == -1)
                RAISE_EXIT("dup2 stderr failure")

        if (setResLimit(runobj) == -1)
            RAISE_EXIT(last_limit_err)

        if (runobj->runner != -1)
            if (setuid(runobj->runner))
                RAISE_EXIT("setuid failure")

        // if (runobj->trace)
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
            RAISE_EXIT("TRACEME failure")

        execvp(runobj->args[0], (char * const *) runobj->args);

        RAISE_EXIT("execvp failure")
    } else {
        int r;
        char errbuffer[100] = { 0 };

        close(fd_err[1]);
        r = read(fd_err[0], errbuffer, 90);
        close(fd_err[0]);
        if (r > 0) {
            waitpid(pid, NULL, WNOHANG);
            RAISE(errbuffer);
            return -1;
        }

        if (runobj->trace)
            r = traceLoop(runobj, rst, pid);
        else
            r = waitExit(runobj, rst, pid);

        if (r)
            RAISE1(last_run_err);
        return 0;
    }
}
