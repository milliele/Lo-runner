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

#include "diff.h"
#include <sys/mman.h>

int equalStr(const char *s, const char *s2) {
    while (*s && *s2) {
        if (*s++ != *s2++) {
            return 1;
        }
    }

    return 0;
}

#define RETURN(rst) {*result = rst; *samelines = same_lines; *totallines = total_lines; return 0;}
int checkDiff(int rightout_fd, int userout_fd, int *result, int *samelines, int *totallines) {
    char *userout, *rightout;
    const char *cuser, *cright, *end_user, *end_right;
    int same_lines = 0, total_lines = 0;

    off_t userout_len, rightout_len;
    userout_len = lseek(userout_fd, 0, SEEK_END);
    rightout_len = lseek(rightout_fd, 0, SEEK_END);

    if (userout_len == -1 || rightout_len == -1)
        RAISE1("lseek failure");

    if (userout_len >= MAX_OUTPUT)
        RETURN(OLE);
      /*OLE means the user output text is longer than the right output * 2*/
    if (userout_len > rightout_len * 2)
        RETURN(OLE);

    lseek(userout_fd, 0, SEEK_SET);
    lseek(rightout_fd, 0, SEEK_SET);
    
    // userout_len === 0 && rightout_len == 0
    if ((userout_len && rightout_len) == 0) {
        if (userout_len || rightout_len)
            RETURN(WA)
        else
            RETURN(AC)
    }

    // open 'userout' file-mapping
    if ((userout = (char*) mmap(NULL, userout_len, PROT_READ | PROT_WRITE,
            MAP_PRIVATE, userout_fd, 0)) == MAP_FAILED) {
        RAISE1("mmap userout filure");
    }

    // open 'rightout' file-mapping
    if ((rightout = (char*) mmap(NULL, rightout_len, PROT_READ | PROT_WRITE,
            MAP_PRIVATE, rightout_fd, 0)) == MAP_FAILED) {
        munmap(userout, userout_len);
        RAISE1("mmap right filure");
    }
    
    // if userout_len equal rightout_len and all characters are the same, return AC
    if ((userout_len == rightout_len) && equalStr(userout, rightout) == 0) {
        munmap(userout, userout_len);
        munmap(rightout, rightout_len);
        RETURN(AC);
    }

    cuser = userout;    // char pointer: cursor of the user's output, at start
    cright = rightout;  // char pointer: cursor of the right's output, at start
    end_user = userout + userout_len;            // pointer: end of the user's output
    end_right = rightout + rightout_len;         // pointer: end of the right's output

    
    while ((cuser < end_user) && (cright < end_right)) {
        while ((cuser < end_user)
                && (*cuser == ' ' || *cuser == '\n' || *cuser == '\r'
                        || *cuser == '\t'))
            cuser++;
        while ((cright < end_right)
                && (*cright == ' ' || *cright == '\n' || *cright == '\r'
                        || *cright == '\t'))
            cright++;
        if (cuser == end_user || cright == end_right)
            break;
        if (*cuser != *cright)
            break;
        cuser++;
        cright++;
    }
    while ((cuser < end_user)
            && (*cuser == ' ' || *cuser == '\n' || *cuser == '\r'
                    || *cuser == '\t'))
        cuser++;
    while ((cright < end_right)
            && (*cright == ' ' || *cright == '\n' || *cright == '\r'
                    || *cright == '\t'))
        cright++;
        
    // after that, if cuser == end_user and cright == end_right, means the result is PE
    if (cuser == end_user && cright == end_right) {
        munmap(userout, userout_len);
        munmap(rightout, rightout_len);
        RETURN(PE);
    }
    
    // now, check each line, and find how many lines different
    
    cuser = userout;    // char pointer: cursor of the user's output, at start
    cright = rightout;  // char pointer: cursor of the right's output, at start
    
    // 检查参考数据不为空的行数总计有多少行
    
    while (cright < end_right){
        int nright = 0;
        while((cright++ < end_right) && (*cright != '\n'))
            nright++;
        if(nright > 0)
            total_lines++;
    }
    
    cright = rightout;
    
    while ((cuser < end_user) && (cright < end_right)){
        int nright = 0, nuser = 0;
        // 检查答案和输出，查找\n，记下偏移值
        while(((cright + nright) < end_right) && (*(cright + nright) != '\n'))
            nright++;
        while(((cuser + nuser) < end_user) && (*(cuser + nuser) != '\n'))
            nuser++;
            
        
        
        // 如果两行偏移值不同或者为0，则不用处理了
        if(nuser == 0 || nright == 0 || nuser != nright){
            cright += (nright + 1); // +1 是要计算多一个\n
            cuser += (nuser + 1);
            continue;
        }
        
        
        int ri = 0, ui = 0;
        while((ri < nright) && (ui < nuser)){
            while ((ri < nright) && (*(cright+ri) == ' ' || *(cright+ri) == '\t' || *(cright+ri) == '\r' ))
                ri++;
            while ((ui < nuser) && (*(cuser+ui) == ' ' || *(cuser+ui) == '\t' || *(cuser+ui) == '\r' ))
                ui++;
            if(ri == nright || ui == nuser) break;
            if(*(cuser+ui) != *(cright+ri)) break;
            ri++;
            ui++;
        }
        
        if(ri == nright && ui == nuser){
            same_lines++;
        }
        
        cright += (ri + 1);
        cuser += (ui + 1);
    }
    
    


    munmap(userout, userout_len);
    munmap(rightout, rightout_len);
    RETURN(WA);
}
