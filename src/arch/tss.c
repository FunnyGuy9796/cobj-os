#include "tss.h"
#include "../misc/printf.h"

tss_t tss = {0};

void tss_init() {
    tss.iopb = sizeof(tss_t);
}

void tss_set_rsp0(uint64_t rsp) {
    tss.rsp0 = rsp;
}