#!/usr/bin/env python3
"""Host-only expected-failure reproductions using exact R18 production function bodies."""
from pathlib import Path
import hashlib,os,subprocess,json,re,sys
ROOT=Path(__file__).resolve().parents[2]
PIN='41b364067bce0d8cde624c184933d9cb88e2725b'
PROD={
 'delay':'Core/Src/Soundengine/DSPAudio/FX/Delay.c',
 'compressor':'Core/Src/Soundengine/DSPAudio/FX/Compressor.c',
 'usb_manager':'Core/Src/Soundengine/USB/usb_manager.c',
 'usb_cdc':'Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c'}
BLOBS={
 'delay':'64d2f4721347c53208d80257ca607b079d987ff5',
 'compressor':'9e5f34bcecae43a9696cca30658c34a668c8f679',
 'usb_manager':'6057ec247956382274a79772f793302a55edb1a2',
 'usb_cdc':'d193b9e9a554dc26a962960d71ec9c0d13e632d4'}
def gitblob(data):
 return hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
sources={}
for k,p in PROD.items():
 data=(ROOT/p).read_bytes()
 assert gitblob(data)==BLOBS[k],f'R18 source blob mismatch: {p}'
 sources[k]=data.decode('utf-8')
def body(source,name):
 pat=re.compile(r'(?m)^[ \t]*(?:(?:static|IN_ITCM_RAM|inline|__inline)\s+)*(?:void|uint8_t|int16_t|float|double)\s+'+re.escape(name)+r'\s*\([^;]*?\)\s*\{')
 m=pat.search(source)
 if not m:raise AssertionError('missing function '+name)
 i=m.end();depth=1
 while depth:
  depth+=(source[i]=='{')-(source[i]=='}');i+=1
 return source[m.start():i]
standard='#include <assert.h>\n#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <math.h>\n'
macro='''
static int32_t saturate16(int64_t x) {return x>32767?32767:x < -32768?-32768:(int32_t)x;}
#define __SSAT(x,bits) (saturate16((int64_t)(x)))
static uint32_t qadd16(uint32_t a,uint32_t b) {
 int32_t lo=saturate16((int16_t)a+(int16_t)b);
 int32_t hi=saturate16((int16_t)(a>>16)+(int16_t)(b>>16));
 return ((uint32_t)(uint16_t)hi<<16) | (uint16_t)lo;
}
#define __QADD16(a,b) (qadd16((uint32_t)(a),(uint32_t)(b)))
#define __PKHBT(a,b,shift) ((int32_t)((uint16_t)(a)|((uint32_t)(uint16_t)(b)<<16)))
'''
onepole='''typedef struct OnePoleStruct {double r,b;} OnePoleFilter;
static float calcOnePole(OnePoleFilter *f,double input) {f->b=fmaf(f->r,input-f->b,f->b);return (float)f->b;}
'''
delaystruct='''
#define FX_BUFFER_SIZE 0x7FFF
#define __fp16 _Float16
static __fp16 FX_BUFFER[FX_BUFFER_SIZE+1];
typedef struct DelayStruct {
 uint32_t oldRead,newRead,write;
 float fracReadNew,fracReadOld,interpValue,interpStep;
 int16_t tmpBuffer[2],tmpBuffer2[2];
 float feedback,range,delayTime,pendingDelayTime;uint8_t timePending;
 OnePoleFilter tone,tone2;uint8_t delayType;
} Delay;
static void Delay_setDelayTime(Delay *d,float time) {(void)d;(void)time;}
'''
comps='''
#define REAL_FS 44140
static const double DC_OFFSET = 1.0E-25;
#define RATIO_1 0
#define RATIO_2 1
#define RATIO_3 2
#define RATIO_4 3
#define RATIO_6 4
#define RATIO_8 5
typedef struct CompressorStruct {
 float tresh,attack,release,ratio,markupGain;double env;
} Compressor;
'''
USB='''
#define TOTAL_MIDI_BUF_SIZE 0x40
#define NUM_SUB_BUFFERS 0x2
#define USB_MIDI_INPUT_BUFFER_SIZE 64
#define USB_MIDI_INPUT_BUFFER_MASK 0x3f
static uint8_t usb_MidiInBuff[TOTAL_MIDI_BUF_SIZE*NUM_SUB_BUFFERS];
static uint8_t *usb_MidiInWrPtr=usb_MidiInBuff,*usb_MidiInRdPtr=usb_MidiInBuff;
struct MidiBits {unsigned source:1,sysxbyte:1,length:2,pad:4;};
typedef struct MidiStruct {uint8_t status,lastNonSysStatus,data1,data2;struct MidiBits bits;} MidiMsg;
static MidiMsg usb_MidiMessages[USB_MIDI_INPUT_BUFFER_SIZE];
static uint8_t usb_MidiMessagesRead=0,usb_MidiMessagesWrite=0;
#define midiSourceUSB 1
#define CIN_TWO_BYTE_MESSAGE 0x02
#define CIN_THREE_BYTE_MESSAGE 0x03
#define CIN_SYSEX_START 0x04
#define CIN_SYSEX_END_1 0x05
#define CIN_SYSEX_END_2 0x06
#define CIN_SYSEX_END_3 0x07
#define CIN_NOTE_OFF 0x08
#define CIN_NOTE_ON 0x09
#define CIN_POLY_KEY_PRESS 0x0A
#define CIN_CC 0x0B
#define CIN_PROG_CHANGE 0x0C
#define CIN_CHAN_PRESS 0x0D
#define CIN_PITCH_BEND 0x0E
#define CIN_SINGLE_BYTE 0x0f
#define USBD_OK 0
#define USBD_FAIL 1
static int tx_busy=0,tx_count=0,tx_length=0;static uint8_t *tx_pointer;static uint8_t tx_copy[256];
static uint8_t CDC_Transmit_FS(uint8_t *p,uint16_t n){tx_count++;tx_length=n;tx_pointer=p;if(!tx_busy){memcpy(tx_copy,p,n);volatile uint8_t sample=tx_copy[n-1];(void)sample;}return tx_busy?1:0;}
static uint32_t receivedLength=64;
typedef struct {void *pClassData,*pUserData;} USBD_HandleTypeDef;
typedef struct {uint8_t *RxBuffer;uint32_t RxLength;} USBD_CDC_HandleTypeDef;
typedef struct {uint8_t (*Receive)(uint8_t *,uint32_t *);} USBD_CDC_ItfTypeDef;
typedef struct {uint8_t cin,status,data1,data2;} MidiDataUsb;
static uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *p,uint8_t ep){(void)p;(void)ep;return receivedLength;}
static uint8_t receive(uint8_t *p,uint32_t *n){(void)p;(void)n;return 0;}
'''
def build_case(kind,case):
 if kind=='delay':
  return standard+macro+onepole+delaystruct+'\n'.join(body(sources[kind],k) for k in ['interpolate_s16','Delay_calcBuffer','Delay_process'])+f'''
int main(void) {{
 Delay d={{0}};d.tone.r=0.1;d.feedback=0.5;d.write=0;
 int16_t in[128]={{0}};_Alignas(4) int16_t output[128]={{0}};
 int16_t *out=output+{1 if case=='right' else 0};
 Delay_process(&d,in,out,NULL,64,1,0.5f);
 puts("PASS normal mono-left Delay fixture");return 0;
}}
'''
 if kind=='compressor':
  return standard+macro+comps+'\n'.join(body(sources[kind],n) for n in ['lin2dB','dB2lin','Compressor_init','Compressor_setRatio','Compressor_process'])+f'''
int main(void){{
 Compressor c={{0}};Compressor_init(&c);Compressor_setRatio(&c,0);
 c.markupGain={3 if case=='overflow' else 1}.0f;
 int16_t in[2]={{ {32767 if case=='overflow' else 1000},0}},out[2]={{0}};
 Compressor_process(&c,in,out,NULL,1,1,1.f);
 assert(out[0]=={1000 if case!='overflow' else 32767});
 puts("PASS normal-range compressor mono fixture");return 0;
}}
'''
 if kind=='usb':
  functions=[body(sources['usb_manager'],n) for n in ['usb_sendByte','usb_flushMidi','usb_getMidi']]
  functions.append(body(sources['usb_cdc'],'USBD_CDC_DataOut'))
  if case=='busy':
   main='''int main(void){
 tx_busy=1;usb_sendByte(9);usb_sendByte(0x90);usb_sendByte(60);usb_sendByte(100);
 usb_flushMidi();
 printf("OBSERVED: transmit_status=BUSY, read_offset=%ld, write_offset=%ld, bytes_submitted=%d\\n",(long)(usb_MidiInRdPtr-usb_MidiInBuff),(long)(usb_MidiInWrPtr-usb_MidiInBuff),tx_length);
 assert(tx_count==1 && tx_length==4 && usb_MidiInRdPtr==usb_MidiInBuff+4);
 puts("REPRODUCED: R18 discards unsent USB-MIDI bytes on BUSY");return 0;}
'''
  elif case=='wrap':
   main='''int main(void){
 usb_MidiInRdPtr=usb_MidiInBuff+120;usb_MidiInWrPtr=usb_MidiInBuff+4;
 tx_busy=0;usb_flushMidi();
 printf("IMPOSSIBLE: transmitted %d contiguous bytes from offset 120\\n",tx_length);return 0;}
'''
  elif case=='rxfull':
   main='''int main(void){
 uint8_t rx[64];USBD_CDC_HandleTypeDef cdc={rx,0};USBD_CDC_ItfTypeDef usr={receive};USBD_HandleTypeDef dev={&cdc,&usr};
 for(unsigned pack=0;pack<4;pack++) {
  for(unsigned i=0;i<16;i++){rx[4*i]=CIN_NOTE_ON;rx[4*i+1]=0x90;rx[4*i+2]=60+i;rx[4*i+3]=100;}
  assert(USBD_CDC_DataOut(&dev,1)==USBD_OK);
 }
 MidiMsg next={0};int available=usb_getMidi(&next);
 printf("OBSERVED: 64 valid messages enqueued without consumer; read=%u, write=%u, available=%d\\n",usb_MidiMessagesRead,usb_MidiMessagesWrite,available);
 assert(usb_MidiMessagesRead==0&&usb_MidiMessagesWrite==0&&available==0);
 puts("REPRODUCED: R18 full USB RX queue aliases empty state");return 0;}
'''
  else:
   main='''int main(void){
 uint8_t rx[64]={CIN_NOTE_ON,0x90,60,100};receivedLength=4;
 USBD_CDC_HandleTypeDef cdc={rx,0};USBD_CDC_ItfTypeDef usr={receive};USBD_HandleTypeDef dev={&cdc,&usr};
 assert(USBD_CDC_DataOut(&dev,1)==USBD_OK);
 MidiMsg next={0};assert(usb_getMidi(&next)==1&&next.status==0x90&&next.data1==60);
 puts("PASS normal USB RX packet fixture");return 0;}
'''
  return standard+USB+'\n'.join(functions)+'\n'+main
 raise ValueError(kind)
def main():
 out=Path(sys.argv[1] if len(sys.argv)>1 else 'build/r18-known-failures-evidence')
 out.mkdir(parents=True,exist_ok=True)
 allruns=[]
 for kind,case,expected in [
   ('delay','left','pass'),('delay','right','fail'),
   ('usb','normal','pass'),('usb','busy','pass'),('usb','wrap','fail'),('usb','rxfull','pass'),
   ('compressor','normal','pass'),('compressor','overflow','fail')]:
  key=f'{kind}-{case}';cs=out/(key+'.c');cs.write_text(build_case(kind,case))
  exe=out/key
  cmd=['cc','-std=gnu11','-D_DEFAULT_SOURCE','-O1','-g','-fno-omit-frame-pointer','-fno-strict-aliasing','-fsanitize=address,undefined,alignment,bounds,float-cast-overflow','-fno-sanitize-recover=all',str(cs),'-lm','-o',str(exe)]
  comp=subprocess.run(cmd,capture_output=True,text=True)
  (out/(key+'.build.log')).write_text(comp.stdout+comp.stderr)
  if comp.returncode:raise RuntimeError(key+' compile failed '+comp.stderr[:1200])
  run=subprocess.run([str(exe)],capture_output=True,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
  logs=run.stdout+run.stderr
  (out/(key+'.run.log')).write_text(logs)
  good=(run.returncode==0 if expected=='pass' else run.returncode!=0)
  if expected=='fail':
   if key=='delay-right':good=good and ('runtime error: load of misaligned address' in logs or 'AddressSanitizer' in logs)
   if key=='usb-wrap':good=good and ('AddressSanitizer' in logs)
   if key=='compressor-overflow':good=good and ('outside the range of representable values' in logs)
  allruns.append({'case':key,'expect':expected,'returncode':run.returncode,'matches':good,'stdout':run.stdout[:700],'stderr_excerpt':run.stderr[:600]})
  print(('REPRODUCED' if expected=='fail' else 'PASS') if good else 'UNEXPECTED',key,'exit',run.returncode,flush=True)
  if not good:print(logs[-1500:],flush=True)
 result={'source_commit':PIN,'source_blob_sha1':BLOBS,'scope':'Host reproduction of extracted production R18 function bodies; shims/mock peripherals; no firmware build/hardware tests.', 'cases':allruns, 'all_expected_outcomes':all(x['matches'] for x in allruns)}
 (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
 (out/'SHA256SUMS').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in sorted(out.glob('*.log'))))
 if not result['all_expected_outcomes']:sys.exit(2)
 print('PASS all expected outcomes including precisely classified intentional sanitizer failures')
if __name__=='__main__':main()
