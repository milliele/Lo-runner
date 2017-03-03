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

#ifndef __LO_GCC_HEADER
#define __LO_GCC_HEADER

#include <Python.h>
#include <sys/types.h>
#define CALLS_MAX 400
#define MAX_OUTPUT 100000000
#define SJ_DISABLED 0
#define SJ_CHECKER 1
#define SJ_INTERACTIVE 2


enum JUDGE_RESULT {
    AC=0,   //0 Accepted
    PE,	    //1 Presentation Error
    TLE,	//2 Time Limit Exceeded
    MLE,	//3 Memory Limit Exceeded
    WA,	    //4 Wrong Answer
    RE,	    //5 Runtime Error
    OLE,	//6 Output Limit Exceeded
    CE,	    //7 Compile Error
    SE,     //8 System Error
    //RJ,     //9 Rejudge
    SPJERR1=10,    // 10 Special Judger Time OUT
    SPJERR2=11,    // 11 Special Judger ERROR
    SPJFIN=12,    // 12 Special Judger Finish, Need Standard Checkup
};

struct Result {
    int judge_result; //JUDGE_RESULT
    int time_used, memory_used;
    int re_signum;
    int re_call;
    const char* re_file;
    int re_file_flag;
};

struct Runobj {
    PyObject *files;
    u_char inttable[CALLS_MAX];
    char * const* args;

    int fd_in, fd_out, fd_err;

    int time_limit, memory_limit;
    int runner;
    int trace;

    int special_judge;              // 0: disable ; 1: checker ; 2: interactive
    char * const* special_judge_checker;

    // ======= checker mode（检查、修正模式） =======
    // special_judge_checker数组定义如下：[0]: checker program; [1]: In File; [2]: Out File; [3]: Answer File
    // 判题机将会执行目标程序，目标程序运行结束后，将会执行检查程序 (最大时间:30000ms)
    // 执行程序时给定输入文件，输出文件，目标程序输出文件，checker将检查并且在stdin里输出
    // CHECKER_AC=0,  CHECKER_PE=1, CHECKER_WA=2, CHECKER_OLE=6 或者 CHECK_CONVERTED=15
    // 如果输出 CHECK_CONVERTED，意味着您的checker将仅对目标程序的应答数据进行处理，检查器将继续执行传统的数据检查程序。（注意，PE将被强制忽略）
    // 如果不是这样，那么系统将根据你的checker返回的结果作为最终的评判结果
    // ======= interactive 交互判题模式  =======
    // special_judge_checker数组定义如下：[0]: checker program; [1]: In File; [2]: Out File; [3] Run Result File
    // 判题机将会在执行目标程序的同时执行检查程序，他们共有的时间是都是 TIME_LIMIT
    // 交互评测模式将把检查程序和目标程序的stdin和stdout关联起来，让他们相互交流
    // 运行结束后，检查程序将结果写入check_result_file，由判题机读取并且处理
};

#define RAISE(msg) PyErr_SetString(PyExc_Exception,msg);

#define RAISE0(msg) \
{PyErr_SetString(PyExc_Exception,msg); return NULL;}

#define RAISE1(msg) \
{PyErr_SetString(PyExc_Exception,msg); return -1;}


#if PY_MAJOR_VERSION >= 3
#define IS_PY3
#endif

#endif
