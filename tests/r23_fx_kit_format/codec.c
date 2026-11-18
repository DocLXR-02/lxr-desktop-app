/* Original versioned FX kit extension codec: validity and fail-closed loading.
 * This test alone does not verify filesystem transactions or kit RAM switching.
 */
#include "Preset/FxKitFormat.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    uint8_t payload[FX_KIT_PAYLOAD_BYTES]={0}, block[FX_KIT_EXTENSION_BYTES];
    uint8_t copy[FX_KIT_EXTENSION_BYTES], recovered[FX_KIT_PAYLOAD_BYTES];
    unsigned scenarios=0;
    for (unsigned seed=0;seed<4096;seed++) {
        for (unsigned e=0;e<4;e++) {
            unsigned p=e*FX_KIT_FIELDS_PER_EFFECT;
            for(unsigned k=0;k<4;k++)payload[p+k]=(uint8_t)((seed*13u+e*17u+k*29u)&127u);
            payload[p+4]=(uint8_t)((seed*5u+e*7u)%101u);
            payload[p+5]=(uint8_t)((seed+e)%6u);
        }
        fxKit_writeBlock(block,payload);
        assert(fxKit_validate(block));
        fxKit_readValues(recovered,block);
        assert(!memcmp(recovered,payload,sizeof(payload)));
        scenarios++;
        /* A wrong CRC, payload length, version, output or wet must be rejected. */
        memcpy(copy,block,sizeof(copy));copy[7]^=0x11;assert(!fxKit_validate(copy));
        memcpy(copy,block,sizeof(copy));copy[4]=2;assert(!fxKit_validate(copy));
        memcpy(copy,block,sizeof(copy));copy[5]=23;assert(!fxKit_validate(copy));
        memcpy(copy,block,sizeof(copy));copy[FX_KIT_EXTENSION_BYTES-1]^=1;assert(!fxKit_validate(copy));
        memcpy(copy,block,sizeof(copy));copy[1]='z';assert(!fxKit_validate(copy));
        memcpy(copy,block,sizeof(copy));copy[FX_KIT_HEADER_BYTES+4]=101;
        fxKit_writeBlock(block,copy+FX_KIT_HEADER_BYTES);
        assert(!fxKit_validate(block));
        memcpy(copy,block,sizeof(copy));
        copy[FX_KIT_HEADER_BYTES+5]=6;
        fxKit_writeBlock(block,copy+FX_KIT_HEADER_BYTES);
        assert(!fxKit_validate(block));
    }
    assert(!fxKit_validate(NULL));
    printf("PASS %u valid format roundtrips and malformed-header/content probes\n",scenarios);
    return 0;
}
