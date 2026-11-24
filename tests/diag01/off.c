#include <assert.h>
#include "safety/runtimeDiagnostics.h"
int main(void) {
    unsigned count=0;
    LXR_DIAG_INIT();
    LXR_DIAG_START(begin);
    LXR_DIAG_END(0,begin);
    LXR_DIAG_CALLBACK(0,++count);
    assert(count==0);
    LXR_DIAG_REARM(0,++count);
    assert(count==1);
    return 0;
}
