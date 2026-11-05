/*
 * usb_manager.c
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 *
 *
 *  Created on: 26.10.2012
 *      Author: Julian Schmidt
 */
#include "Soundengine/USB/usb_manager.h"
#include <string.h>
#include "usbd_cdc_if.h"

/* USB transfers finish asynchronously; keep accepted packets in the ring
 * until the CDC IN endpoint has completed (including a possible ZLP). */
extern USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t usb_MidiTxPending = 0;
static uint8_t usb_MidiTxPendingBytes = 0;
volatile uint32_t usb_MidiTxDropped = 0;




enum {
	USB_NOT_DETECTED,
	USB_DETECTED,
};
//USBD_HandleTypeDef           USB_OTG_dev;
//-------------------------------------------------------------------------------
#if 0
void usb_init()
{
#if HAL_USB
	  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
	  {
	    Error_Handler();
	  }
	  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK)
	  {
	    Error_Handler();
	  }
	  if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK)
	  {
	    Error_Handler();
	  }
	  if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
	  {
	    Error_Handler();
	  }
#else

	  usb_start();
#endif
}
//-------------------------------------------------------------------------------
void usb_stop()
{
	//stop usb port
	USBD_DeInit(&USB_OTG_dev);
}
#endif
//-------------------------------------------------------------------------------
void usb_tick()
{
	usb_flushMidi();
}
//-------------------------------------------------------------------------------
#if 0
void usb_start()
{
	//zero init the usb receive buffer
	memset(usb_MidiMessages,0, sizeof(MidiMsg)*USB_MIDI_INPUT_BUFFER_SIZE);

	//start usb port
	 USBD_Init(&USB_OTG_dev,
	            &USR_desc,
				DEVICE_FS);

	 if (USBD_RegisterClass(&USB_OTG_dev, &MIDI_cb) != USBD_OK)

	 // if (USBD_CDC_RegisterInterface(&USB_OTG_dev, &USBD_Interface_fops_FS) != USBD_OK)
	  {
		//Error_Handler();
	  }
	  if (USBD_Start(&USB_OTG_dev) != USBD_OK)
	  {
		//Error_Handler();
	  }
}
#endif
//-------------------------------------------------------------------------------
/*
 * writes a byte to the Usb Tx buffer
 * Will be send out by the usb_flushMidi() call
 */
/* The 128-byte TX ring contains complete 4-byte USB-MIDI events only.
 * Leave one 4-byte slot empty to distinguish full from empty. All TX
 * producers and usb_flushMidi run on the existing foreground audio loop. */
void usb_sendMidi(MidiMsg msg)
{
    const uint16_t capacity = TOTAL_MIDI_BUF_SIZE * NUM_SUB_BUFFERS;
    const uint16_t rd = (uint16_t)(usb_MidiInRdPtr - usb_MidiInBuff);
    const uint16_t wr = (uint16_t)(usb_MidiInWrPtr - usb_MidiInBuff);
    const uint16_t freeBytes = (uint16_t)((rd + capacity - wr - 1u) % capacity);
    if (freeBytes < 4u)
    {
        usb_MidiTxDropped++;
        return; /* Drop newest complete event; never overwrite in-flight data. */
    }

    const uint8_t packet[4] = { msg.status >> 4, msg.status, msg.data1, msg.data2 };
    uint16_t next = wr;
    for (uint8_t i = 0; i < 4u; ++i)
    {
        usb_MidiInBuff[next] = packet[i];
        next = (uint16_t)((next + 1u) % capacity);
    }
    usb_MidiInWrPtr = usb_MidiInBuff + next;
}
//------------------------------------------------------------------------------
void usb_flushMidi(void)
{
    const uint16_t capacity = TOTAL_MIDI_BUF_SIZE * NUM_SUB_BUFFERS;
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
    if (hcdc == NULL || hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED)
    {
        /* A disconnected transfer has no completion to wait for. Preserve
         * queued events so that a subsequent connection can retry them. */
        usb_MidiTxPending = 0;
        return;
    }

    if (usb_MidiTxPending != 0u)
    {
        if (hcdc->TxState != 0u) return; /* CDC still owns this memory. */
        const uint16_t rd = (uint16_t)(usb_MidiInRdPtr - usb_MidiInBuff);
        usb_MidiInRdPtr = usb_MidiInBuff + ((rd + usb_MidiTxPendingBytes) % capacity);
        usb_MidiTxPending = 0u;
    }
    if (hcdc->TxState != 0u) return;

    const uint16_t rd = (uint16_t)(usb_MidiInRdPtr - usb_MidiInBuff);
    const uint16_t wr = (uint16_t)(usb_MidiInWrPtr - usb_MidiInBuff);
    const uint16_t queued = (uint16_t)((wr + capacity - rd) % capacity);
    if (queued == 0u) return;

    /* The USB class retains the pointer until DataIn clears TxState. Only
     * submit a contiguous, 4-byte-aligned event sequence (at most 64 bytes). */
    uint16_t len = queued;
    if (len > TOTAL_MIDI_BUF_SIZE) len = TOTAL_MIDI_BUF_SIZE;
    if (len > capacity - rd) len = capacity - rd;
    len &= (uint16_t)~3u;
    if (len == 0u) return;

    if (CDC_Transmit_FS(usb_MidiInRdPtr, len) == USBD_OK)
    {
        usb_MidiTxPendingBytes = (uint8_t)len;
        usb_MidiTxPending = 1u;
    }
    /* BUSY/FAIL: the queued bytes and the read pointer remain untouched. */
}
//------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
uint8_t usb_getMidi(MidiMsg* msg)
{
	if(usb_MidiMessagesRead != usb_MidiMessagesWrite)
	{
		//we have unprocessed messages in the queue
		*msg = usb_MidiMessages[usb_MidiMessagesRead];

        /* Publish freed RX slot after copying its complete message. */
        __DMB();
        usb_MidiMessagesRead = (uint8_t)((usb_MidiMessagesRead + 1u) & USB_MIDI_INPUT_BUFFER_MASK);

		return 1; //return OK
	}
return 0; //no messages
}

