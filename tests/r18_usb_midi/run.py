#!/usr/bin/env python3
"""Bounded original-R18 vs candidate USB-MIDI host reproductions.

Extract verbatim production functions from both source versions.
Mock USB IRQ completion and CDC ownership; not a hardware/ARM timing test.
"""
from pathlib import Path
import hashlib, json, os, re, subprocess, sys
ROOT = Path(__file__).resolve().parents[2]
BASE = "41b364067bce0d8cde624c184933d9cb88e2725b"
PATHS = {
    "manager": "Core/Src/Soundengine/USB/usb_manager.c",
    "cdc": "Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c",
    "header": "Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h",
}
ORIGINAL = {
    "manager": "6057ec247956382274a79772f793302a55edb1a2",
    "cdc": "d193b9e9a554dc26a962960d71ec9c0d13e632d4",
    "header": "27cc529e8cd2a78cb8117017b0247ab096c0af26",
}
CANDIDATE = {
    "manager": "7f30ed0fcdb433f280b98bb238879cb28834b585",
    "cdc": "ef33e3e2941db2a8a72705d66d419c29966f4d47",
    "header": "56bb9165ae5d2bf0134a55a4a95652bc706aef77",
}
def blob(raw):
    return hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\x00" + raw).hexdigest()
def source(which, key):
    p = PATHS[key]
    if which == "candidate":
        raw = (ROOT / p).read_bytes()
    else:
        raw = subprocess.check_output(["git", "show", BASE + ":" + p], cwd=ROOT)
    assert blob(raw) == (CANDIDATE if which == "candidate" else ORIGINAL)[key], (which, key)
    return raw.decode("utf-8")
def function(text, name):
    pattern = r"(?m)^[ \t]*(?:static\s+)?(?:uint8_t|void)\s+" + re.escape(name) + r"\s*\([^;]*?\)\s*\{"
    m = re.search(pattern, text)
    assert m, name
    i = m.end()
    depth = 1
    while depth:
        depth += (text[i] == "{") - (text[i] == "}")
        i += 1
    return text[m.start():i]

PRELUDE = r"""
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define __DMB() __asm__ __volatile__("" ::: "memory")
#define USB_MIDI_INPUT_BUFFER_SIZE 64u
#define USB_MIDI_INPUT_BUFFER_MASK 0x3fu
#define TOTAL_MIDI_BUF_SIZE 64u
#define NUM_SUB_BUFFERS 2u
#define CDC_DATA_FS_MAX_PACKET_SIZE 64u
#define USBD_STATE_CONFIGURED 3u
#define USBD_OK 0u
#define USBD_BUSY 1u
#define USBD_FAIL 2u
#define midiSourceUSB 1u
#define CIN_TWO_BYTE_MESSAGE 0x02
#define CIN_THREE_BYTE_MESSAGE 0x03
#define CIN_SYSEX_START 0x04
#define CIN_SYSEX_END_1 0x05
#define CIN_SYSEX_END_2 0x06
#define CIN_SYSEX_END_3 0x07
#define CIN_NOTE_OFF 0x08
#define CIN_NOTE_ON 0x09
#define CIN_POLY_KEY_PRESS 0x0a
#define CIN_CC 0x0b
#define CIN_PROG_CHANGE 0x0c
#define CIN_CHAN_PRESS 0x0d
#define CIN_PITCH_BEND 0x0e
#define CIN_SINGLE_BYTE 0x0f
typedef struct {unsigned source:1,sysxbyte:1,length:2,pad:4;} MidiBits;
typedef struct {uint8_t status,lastNonSysStatus,data1,data2;MidiBits bits;} MidiMsg;
typedef struct {uint8_t cin,status,data1,data2;} MidiDataUsb;
typedef struct {uint8_t* RxBuffer;uint32_t RxLength;volatile uint32_t TxState;} USBD_CDC_HandleTypeDef;
typedef struct {uint8_t (*Receive)(uint8_t *,uint32_t *);} USBD_CDC_ItfTypeDef;
typedef struct {void *pClassData,*pUserData;uint8_t dev_state;} USBD_HandleTypeDef;
static uint8_t usb_MidiInBuff[TOTAL_MIDI_BUF_SIZE*NUM_SUB_BUFFERS];
static uint8_t *usb_MidiInWrPtr=usb_MidiInBuff,*usb_MidiInRdPtr=usb_MidiInBuff;
static MidiMsg usb_MidiMessages[USB_MIDI_INPUT_BUFFER_SIZE];
static volatile uint8_t usb_MidiMessagesRead,usb_MidiMessagesWrite;
static volatile uint32_t usb_MidiRxDropped;
static USBD_CDC_HandleTypeDef tx_class;
USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t *tx_ptr;
static uint16_t tx_len;
static uint32_t tx_submitted, tx_finished, rx_rearmed;
static uint8_t force_busy, wire[8192];
static size_t wire_len;
static uint32_t incoming_length;
static uint8_t incoming[64];
static uint8_t rx_receive(uint8_t *buf,uint32_t *len){(void)buf;(void)len;rx_rearmed++;return USBD_OK;}
static uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev,uint8_t ep){(void)pdev;(void)ep;return incoming_length;}
static uint8_t CDC_Transmit_FS(uint8_t *buf,uint16_t length) {
  if(force_busy || tx_class.TxState) return USBD_BUSY;
  if(length==0 || (length%4) ||
     buf < usb_MidiInBuff || (size_t)(buf-usb_MidiInBuff)+length>sizeof usb_MidiInBuff) {
    fprintf(stderr,"REPRODUCED invalid TX span: length=%u offset=%ld\n",length,(long)(buf-usb_MidiInBuff));
    abort();
  }
  tx_ptr=buf;tx_len=length;tx_class.TxState=1;tx_submitted++;
  return USBD_OK;
}
static void complete_tx(void) {
  assert(tx_class.TxState==1 && tx_ptr && tx_len);
  assert(wire_len+tx_len<sizeof wire);
  memcpy(wire+wire_len,tx_ptr,tx_len);
  wire_len+=tx_len;tx_finished++;
  tx_class.TxState=0;tx_ptr=NULL;tx_len=0;
}
void usb_flushMidi(void);
void usb_sendMidi(MidiMsg msg);
uint8_t usb_getMidi(MidiMsg *msg);
static void reset_fixture(void) {
  memset(usb_MidiInBuff,0,sizeof usb_MidiInBuff);
  memset(usb_MidiMessages,0,sizeof usb_MidiMessages);
  usb_MidiInWrPtr=usb_MidiInRdPtr=usb_MidiInBuff;
  usb_MidiMessagesRead=usb_MidiMessagesWrite=0;usb_MidiRxDropped=0;
  tx_class.TxState=0;tx_submitted=tx_finished=rx_rearmed=0;
  hUsbDeviceFS.pClassData=&tx_class;
  hUsbDeviceFS.dev_state=USBD_STATE_CONFIGURED;
  tx_ptr=NULL;tx_len=0;force_busy=0;wire_len=0;
}
static MidiMsg event(unsigned i){
 MidiMsg m={0};m.status=0x90;m.data1=(uint8_t)(i%100);m.data2=(uint8_t)((i+7)%100);return m;
}
static void send_range(unsigned a,unsigned b){
 for(unsigned i=a;i<b;i++) usb_sendMidi(event(i));
}
static void drain(void){
 for(unsigned i=0;i<100;i++){
  usb_flushMidi();
  if(tx_class.TxState)complete_tx();
  else if(usb_MidiInRdPtr==usb_MidiInWrPtr)return;
 }
 assert(!"TX drain exceeded retry limit");
}
static void check_events(unsigned number){
 assert(wire_len==number*4u);
 for(unsigned i=0;i<number;i++){
  MidiMsg m=event(i);
  assert(wire[4*i]==9 && wire[4*i+1]==m.status &&
         wire[4*i+2]==m.data1 && wire[4*i+3]==m.data2);
 }
}
static void send_rx_packet(unsigned first,unsigned n){
 assert(n<=16);
 for(unsigned i=0;i<n;i++){
  incoming[4*i]=CIN_NOTE_ON;incoming[4*i+1]=0x90;
  incoming[4*i+2]=(uint8_t)((first+i)%100);
  incoming[4*i+3]=100;
 }
 incoming_length=n*4;
 USBD_CDC_HandleTypeDef rx_class={incoming,incoming_length,0};
 USBD_CDC_ItfTypeDef iface={rx_receive};
 USBD_HandleTypeDef rxdev={&rx_class,&iface,USBD_STATE_CONFIGURED};
 extern uint8_t USBD_CDC_DataOut(USBD_HandleTypeDef *, uint8_t);
 assert(USBD_CDC_DataOut(&rxdev,1)==USBD_OK);
}
"""
BODY = r"""
int main(int argc,char **argv){
 assert(argc==2);
 reset_fixture();
 const char *scenario=argv[1];
 if(!strcmp(scenario,"normal")){
   send_range(0,4);drain();check_events(4);
   printf("WIRE ");for(unsigned i=0;i<wire_len;i++)printf("%02x",wire[i]);puts("");
 } else if(!strcmp(scenario,"busy")){
   send_range(0,1);
   force_busy=1;usb_flushMidi();
   assert(usb_MidiInRdPtr==usb_MidiInBuff && usb_MidiInWrPtr==usb_MidiInBuff+4);
   assert(tx_submitted==0);
   force_busy=0;drain();check_events(1);
   puts("PASS BUSY retains all four unsent event bytes");
 } else if(!strcmp(scenario,"external-busy")){
   send_range(0,1);tx_class.TxState=1;usb_flushMidi();
   assert(tx_submitted==0 && usb_MidiInRdPtr==usb_MidiInBuff);
   tx_class.TxState=0;drain();check_events(1);
   puts("PASS unrelated CDC transfer busy keeps queue intact");
 } else if(!strcmp(scenario,"inflight")){
   send_range(0,16);usb_flushMidi();assert(tx_class.TxState==1 && tx_len==64);
   uint8_t held[64];memcpy(held,tx_ptr,64);
   send_range(16,31);
   usb_sendMidi(event(31));
   assert(usb_MidiTxDropped==1);
   assert(!memcmp(held,tx_ptr,64));
   usb_flushMidi();assert(tx_submitted==1 && tx_class.TxState==1);
   complete_tx();drain();check_events(31);
   puts("PASS pending TX bytes remain unchanged under FIFO pressure; one whole new event dropped");
 } else if(!strcmp(scenario,"wrap")){
   send_range(0,30);drain();check_events(30);
   assert(usb_MidiInRdPtr==usb_MidiInBuff+120);
   send_range(30,33);drain();check_events(33);
   assert(tx_submitted==4); /* 64 + 56 + 8 + 4 */
   puts("PASS crossing TX wrap at offset 120 sends 8 + 4 bytes without OOB");
 } else if(!strcmp(scenario,"disconnect")){
   send_range(0,1);usb_flushMidi();assert(tx_class.TxState==1);
   hUsbDeviceFS.dev_state=0;usb_flushMidi();
   tx_class.TxState=0;tx_ptr=NULL;tx_len=0; /* aborted transfer */
   hUsbDeviceFS.dev_state=USBD_STATE_CONFIGURED;
   drain();check_events(1);
   puts("PASS aborted disconnection retains pending event for retry");
 } else if(!strcmp(scenario,"rx-normal")){
   send_rx_packet(3,4);MidiMsg m={0};
   for(unsigned i=0;i<4;i++){
     assert(usb_getMidi(&m)==1 && m.status==0x90 &&
            m.data1==i+3 && m.bits.source==midiSourceUSB && m.bits.length==2);
     printf("RX %u %u\n",m.status,m.data1);
   }
   assert(!usb_getMidi(&m) && rx_rearmed==1);
 } else if(!strcmp(scenario,"rx-alias")){
   for(unsigned k=0;k<4;k++)send_rx_packet(16*k,16);
   assert(usb_MidiMessagesRead==usb_MidiMessagesWrite);
   MidiMsg m={0};assert(!usb_getMidi(&m));
   puts("REPRODUCED original 64-event RX-full queue aliases empty");
 } else if(!strcmp(scenario,"rx-full")){
   for(unsigned k=0;k<5;k++)send_rx_packet(16*k,16);
   assert(usb_MidiRxDropped==17);
   unsigned count=0;MidiMsg m={0};
   while(usb_getMidi(&m)){
      assert(count<63 && m.status==0x90 && m.data1==(count%100) &&
             m.bits.source==midiSourceUSB && m.bits.length==2);
      count++;
   }
   assert(count==63 && usb_MidiMessagesRead==usb_MidiMessagesWrite);
   send_rx_packet(80,1);assert(usb_getMidi(&m)==1 && m.data1==80);
   puts("PASS RX full retains 63 oldest complete messages, drops 17 newest and recovers");
 } else if(!strcmp(scenario,"rx-short")){
   incoming[0]=CIN_NOTE_ON;incoming[1]=0x90;incoming[2]=60;
   incoming_length=3;
   USBD_CDC_HandleTypeDef rx_class={incoming,incoming_length,0};
   USBD_CDC_ItfTypeDef iface={rx_receive};
   USBD_HandleTypeDef rxdev={&rx_class,&iface,USBD_STATE_CONFIGURED};
   assert(USBD_CDC_DataOut(&rxdev,1)==USBD_OK);
   MidiMsg m={0};assert(!usb_getMidi(&m) && usb_MidiRxDropped==0);
   send_rx_packet(0,1);assert(usb_getMidi(&m)==1 && m.data1==0);
   puts("PASS truncated packet ignored, following complete event intact");
 } else {fprintf(stderr,"bad scenario %s\n",scenario);return 3;}
 return 0;
}
"""

def fixture(which):
    man=source(which,"manager")
    cdc=source(which,"cdc")
    if which=="candidate":
        pre=man[man.index("extern USBD_HandleTypeDef hUsbDeviceFS;"):
                man.index("void usb_tick()")]
        functions=[function(man,x) for x in
                   ("usb_sendMidi","usb_flushMidi","usb_getMidi")]
    else:
        pre="volatile uint32_t usb_MidiTxDropped = 0;\n"
        functions=[function(man,x) for x in
                   ("usb_sendByte","usb_sendMidi","usb_flushMidi","usb_getMidi")]
    cdc_fn=function(cdc,"USBD_CDC_DataOut").replace("static uint8_t  USBD_CDC_DataOut","uint8_t  USBD_CDC_DataOut",1)
    assert "USBD_CDC_DataOut" in cdc_fn
    return PRELUDE+pre+"\n".join(functions)+"\n"+cdc_fn+"\n"+BODY
def main():
    out=Path(sys.argv[1] if len(sys.argv)>1 else "build/r18-usb-midi-host-evidence")
    out.mkdir(parents=True,exist_ok=True)
    for label in ("original","candidate"):
        c=out/(label+".c");c.write_text(fixture("candidate" if label=="candidate" else "original"))
        cmd=["cc","-std=gnu11","-D_DEFAULT_SOURCE","-O1","-g","-fno-omit-frame-pointer",
             "-fno-strict-aliasing","-fsanitize=address,undefined,bounds,alignment",
             "-fno-sanitize-recover=all",str(c),"-o",str(out/label)]
        build=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        (out/(label+"-compile.log")).write_text(build.stdout+build.stderr)
        assert build.returncode==0,(label,build.stderr[-2500:])
    normal={};rx_normal={}
    cases=[]
    for label,case,expected in [
        ("original","normal",True),
        ("original","rx-normal",True),
        ("original","rx-alias",True),
        ("original","busy",False),
        ("original","wrap",False),
        ("original","rx-full",False),
        ("candidate","normal",True),
        ("candidate","rx-normal",True),
        ("candidate","busy",True),
        ("candidate","external-busy",True),
        ("candidate","inflight",True),
        ("candidate","wrap",True),
        ("candidate","disconnect",True),
        ("candidate","rx-full",True),
        ("candidate","rx-short",True),
    ]:
        env=dict(os.environ,ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",
                 UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        p=subprocess.run([str(out/label),case],stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                         text=True,env=env)
        (out/(label+"-"+case+".log")).write_text(p.stdout+p.stderr)
        correct=((p.returncode==0)==expected)
        if not expected:
            correct &= bool(p.stderr) # original reproductions must fail visibly
        print(("PASS" if expected else "REPRODUCED") if correct else "FAIL",label,case,
              "exit",p.returncode,flush=True)
        if case=="normal" and p.returncode==0:normal[label]=p.stdout
        if case=="rx-normal" and p.returncode==0:rx_normal[label]=p.stdout
        cases.append({"revision":label,"case":case,"expected_pass":expected,
            "exit":p.returncode,"matched":correct,"output":(p.stdout+p.stderr)[-900:]})
    assert normal.get("original")==normal.get("candidate"),"Normal USB wire bytes differ"
    assert rx_normal.get("original")==rx_normal.get("candidate"),"Normal RX MIDI events differ"
    assert all(x["matched"] for x in cases),"At least one validation failed"
    result={"accepted_source":BASE,"original_blobs":ORIGINAL,"candidate_blobs":CANDIDATE,
        "normal_usb_wire_bitidentical":True,"normal_rx_events_bitidentical":True,
        "checks":cases,
        "limits":"Source-extracted USB functions with emulated CDC TxState/DataOut; no actual USB device, IRQ preemption, D-cache, DMA, ARM build or physical MIDI host."}
    (out/"result.json").write_text(json.dumps(result,indent=2)+"\n")
    (out/"SHA256SUMS.txt").write_text("".join(
        hashlib.sha256(p.read_bytes()).hexdigest()+"  "+p.name+"\n"
        for p in sorted(out.glob("*.log")) if p.name != "USB_MIDI_RESULTS.log"))
    print("PASS: original three USB faults, normal TX/RX byte identity, candidate TX/RX lifetime/overflow/wrap/short-packet cases")
if __name__=="__main__":
    main()
