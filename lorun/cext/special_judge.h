#ifndef __LO_SPCJUDGE_HEADER
#define __LO_SPCJUDGE_HEADER
#define CHECKER_AC 0
#define CHECKER_PE 1
#define CHECKER_WA 4
#define CHECKER_OLE 6
#define CHECK_CONVERTED 15

#include "lorun.h"

int special_checker(struct Runobj *runobj, struct Result *rst);
int interactive_judge(struct Runobj *runobj, struct Result *rst);
#endif