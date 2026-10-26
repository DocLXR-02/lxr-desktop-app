/* Simulated lifecycle, not a physical USB test. Production bodies are extracted. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"

USBD_HandleTypeDef hUsbDeviceFS;
USBD_ClassTypeDef USBD_CDC;
uint8_t UserRxBufferFS[2048], UserTxBufferFS[2048];
static unsigned opens, closes, starts, stops, prepares, errors, acknowledgements;
static int fail_open, fail_close, fail_prepare, fail_init, fail_deinit, missing_rx;
static unsigned open_sequence[3], open_sizes[3], reenter;
static uint8_t USBD_CDC_Init(USBD_HandleTypeDef *,uint8_t);
static uint8_t USBD_CDC_DeInit(USBD_HandleTypeDef *,uint8_t);
static int8_t test_init(void);
static int8_t test_deinit(void);
static USBD_CDC_ItfTypeDef fops={.Init=test_init,.DeInit=test_deinit};

static void *forbidden_allocate(size_t size) { (void)size; abort(); }
static void forbidden_free(void *p) { (void)p; abort(); }
#undef USBD_malloc
#undef USBD_free
#define USBD_malloc forbidden_allocate
#define USBD_free forbidden_free
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *p,uint8_t ep,uint8_t type,uint16_t size) {
    (void)p; (void)type;
    if(!(ep&15)) return USBD_OK; /* EP0 opened by actual reset body. */
    open_sequence[opens%3]=ep; open_sizes[opens%3]=size; opens++;
    if(fail_open && --fail_open==0) return USBD_FAIL;
    return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *p,uint8_t ep) {
    (void)p; (void)ep; closes++;
    if(fail_close && --fail_close==0) return USBD_FAIL;
    return USBD_OK;
}
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *p,uint8_t ep,uint8_t *buffer,uint16_t size) {
    assert(p==&hUsbDeviceFS && ep==CDC_OUT_EP && buffer==UserRxBufferFS);
    assert(size==(p->dev_speed==USBD_SPEED_HIGH?CDC_DATA_HS_OUT_PACKET_SIZE:CDC_DATA_FS_OUT_PACKET_SIZE));
    prepares++; return fail_prepare?USBD_FAIL:USBD_OK;
}
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *p) { (void)p; return USBD_OK; }
void USBD_CtlError(USBD_HandleTypeDef *p,USBD_SetupReqTypedef *r) { (void)p;(void)r;errors++; }
USBD_StatusTypeDef USBD_CtlSendStatus(USBD_HandleTypeDef *p) { (void)p;acknowledgements++;return USBD_OK; }
#include "functions.inc"

static void check_zero(void) {
    unsigned char expected[sizeof(cdc_storage)]={0};
    assert(!memcmp(expected,&cdc_storage,sizeof(cdc_storage)));
}
static int8_t test_init(void) {
    assert(hUsbDeviceFS.pClassData==&cdc_storage && cdc_owner==&hUsbDeviceFS && cdc_busy);
    assert((uintptr_t)&cdc_storage % _Alignof(USBD_CDC_HandleTypeDef)==0);
    check_zero(); starts++;
    if(reenter) {
        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
        assert(USBD_CDC_DeInit(&hUsbDeviceFS,1)==USBD_FAIL);
    }
    int8_t ret=CDC_Init_FS();
    if(missing_rx) cdc_storage.RxBuffer=NULL;
    return fail_init?USBD_FAIL:ret;
}
static int8_t test_deinit(void) {
    assert(hUsbDeviceFS.pClassData==&cdc_storage && cdc_owner==&hUsbDeviceFS && cdc_busy);
    stops++;
    if(reenter) {
        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
        assert(USBD_CDC_DeInit(&hUsbDeviceFS,1)==USBD_FAIL);
    }
    return fail_deinit?USBD_FAIL:CDC_DeInit_FS();
}
static void idle(void) {
    assert(cdc_owner==NULL && hUsbDeviceFS.pClassData==NULL && !cdc_busy && !cdc_app_started);
    assert(!hUsbDeviceFS.ep_in[CDC_IN_EP&15].is_used && !hUsbDeviceFS.ep_in[CDC_CMD_EP&15].is_used && !hUsbDeviceFS.ep_out[CDC_OUT_EP&15].is_used);
    check_zero();
}
static void reset_device(unsigned speed) {
    assert(!cdc_owner);
    memset(&hUsbDeviceFS,0,sizeof(hUsbDeviceFS));
    hUsbDeviceFS.pClass=&USBD_CDC; hUsbDeviceFS.pUserData=&fops;
    hUsbDeviceFS.dev_speed=speed;hUsbDeviceFS.dev_state=USBD_STATE_ADDRESSED;
    opens=closes=starts=stops=prepares=errors=acknowledgements=0;
    fail_open=fail_close=fail_prepare=fail_init=fail_deinit=missing_rx=0; reenter=1;
}
static void configure(unsigned value) {
    USBD_SetupReqTypedef req={.wValue=value};USBD_SetConfig(&hUsbDeviceFS,&req);
}
int main(void) {
    (void)forbidden_allocate; (void)forbidden_free;
    USBD_CDC.Init=USBD_CDC_Init;USBD_CDC.DeInit=USBD_CDC_DeInit;
    reset_device(USBD_SPEED_FULL);
    assert(USBD_CDC_Init(NULL,1)==USBD_FAIL && USBD_CDC_DeInit(NULL,1)==USBD_FAIL);
    hUsbDeviceFS.pUserData=NULL; assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
    hUsbDeviceFS.pUserData=&fops;fops.Init=NULL;assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);fops.Init=test_init;
    fops.DeInit=NULL;assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);fops.DeInit=test_deinit;
    hUsbDeviceFS.pClassData=UserRxBufferFS;
    assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL && USBD_CDC_DeInit(&hUsbDeviceFS,1)==USBD_FAIL);
    assert(hUsbDeviceFS.pClassData==UserRxBufferFS);hUsbDeviceFS.pClassData=NULL;
    hUsbDeviceFS.ep_in[CDC_IN_EP&15].is_used=1;assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
    hUsbDeviceFS.ep_in[CDC_IN_EP&15].is_used=0;assert(!opens && !starts);idle();
    for(unsigned speed=0;speed<2;speed++) for(unsigned cycle=0;cycle<1000;cycle++) {
        reset_device(speed?USBD_SPEED_HIGH:USBD_SPEED_FULL);
        configure(1); assert(!errors && starts==1 && opens==3 && prepares==1);
        assert(open_sequence[0]==CDC_IN_EP && open_sequence[1]==CDC_OUT_EP && open_sequence[2]==CDC_CMD_EP);
        assert(open_sizes[0]==(speed?CDC_DATA_HS_IN_PACKET_SIZE:CDC_DATA_FS_IN_PACKET_SIZE));
        assert(open_sizes[1]==(speed?CDC_DATA_HS_OUT_PACKET_SIZE:CDC_DATA_FS_OUT_PACKET_SIZE));
        assert(open_sizes[2]==CDC_CMD_PACKET_SIZE);
        assert(cdc_storage.RxBuffer==UserRxBufferFS && cdc_storage.TxBuffer==UserTxBufferFS);
        configure(1);assert(opens==3 && starts==1); /* core ACKs unchanged configuration */
        void *live=hUsbDeviceFS.pClassData;
        cdc_storage.CmdOpCode=37;cdc_storage.TxState=1;cdc_storage.RxLength=100;
        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL && hUsbDeviceFS.pClassData==live);
        assert(cdc_storage.CmdOpCode==37 && cdc_storage.TxState==1 && cdc_storage.RxLength==100);
        USBD_HandleTypeDef other={.pClass=&USBD_CDC,.pUserData=&fops};
        assert(USBD_CDC_Init(&other,1)==USBD_FAIL && USBD_CDC_DeInit(&other,1)==USBD_FAIL && !other.pClassData);
        if(cycle%4==0) configure(0);
        else if(cycle%4==1) assert(USBD_LL_Reset(&hUsbDeviceFS)==USBD_OK);
        else if(cycle%4==2) assert(USBD_LL_DevDisconnected(&hUsbDeviceFS)==USBD_OK);
        else assert(USBD_Stop(&hUsbDeviceFS)==USBD_OK);
        idle();assert(closes==3 && stops==1);
        assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK && closes==3 && stops==1);
        hUsbDeviceFS.dev_state=USBD_STATE_ADDRESSED;configure(1);
        assert(starts==2 && hUsbDeviceFS.pClassData==live); /* stable storage, freshly zeroed */
        configure(0);idle();
    }
    puts("PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing");
    unsigned failures=0;
    for(unsigned kind=0;kind<6;kind++) {
        reset_device(USBD_SPEED_FULL);
        if(kind<3) fail_open=kind+1;
        if(kind==3) fail_init=1;
        if(kind==4) missing_rx=1;
        if(kind==5) fail_prepare=1;
        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);idle();
        assert(opens==closes);fail_open=fail_init=missing_rx=fail_prepare=0;
        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);
        assert(USBD_CDC_DeInit(&hUsbDeviceFS,1)==USBD_OK);idle();failures++;
    }
    for(unsigned close=1;close<=3;close++) {
        reset_device(USBD_SPEED_FULL);assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);
        void *live=hUsbDeviceFS.pClassData;fail_close=close;
        assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_FAIL && cdc_owner && hUsbDeviceFS.pClassData==live);
        assert(!cdc_busy && !stops && cdc_app_started);
        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
        assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
    }
    reset_device(USBD_SPEED_FULL);assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);fail_deinit=1;
    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_FAIL && cdc_owner && hUsbDeviceFS.pClassData);
    assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
    fail_deinit=0;assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
    reset_device(USBD_SPEED_FULL);fail_prepare=1;fail_close=1;
    assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL && cdc_owner && hUsbDeviceFS.pClassData);
    fail_prepare=0;assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
    assert(USBD_LL_Reset(&hUsbDeviceFS)==USBD_OK);idle();failures++;
    reset_device(USBD_SPEED_FULL);assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);
    hUsbDeviceFS.pClassData=UserRxBufferFS;
    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_FAIL && !closes && cdc_owner);
    hUsbDeviceFS.pClassData=&cdc_storage;
    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
    reset_device(USBD_SPEED_FULL);assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);
    hUsbDeviceFS.pUserData=NULL;
    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_FAIL && cdc_owner && hUsbDeviceFS.pClassData);
    hUsbDeviceFS.pUserData=&fops;
    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
    reset_device(USBD_SPEED_FULL);fail_init=fail_deinit=1;
    assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL && cdc_owner && cdc_app_started);
    fail_init=fail_deinit=0;
    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
    reset_device(USBD_SPEED_FULL);configure(USBD_MAX_NUM_CONFIGURATION+1);assert(errors==1 && !opens);idle();
    printf("PASS USB failures: %u rollback/retention/retry scenarios plus invalid requests; no allocator calls\n",failures);
    printf("Host object: size=%zu alignment=%zu (target size/alignment checked separately in ELF)\n",sizeof(cdc_storage),_Alignof(USBD_CDC_HandleTypeDef));
}
