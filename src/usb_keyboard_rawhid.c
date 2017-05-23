/* Teensy RawHID example and
 * USB Keyboard Example for Teensy USB Development Board merged.
 * http://www.pjrc.com/teensy/usb_keyboard.html
 * http://www.pjrc.com/teensy/rawhid.html
 * Copyright (c) 2009 PJRC.COM, LLC
 * 2015 Henri Tuhola adjusted this code to get a dual-driver for ergodox
 * mechanical keyboard.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above description, website URL and copyright notice and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Version 1.0: Initial Release
// Version 1.1: fixed bug in analog
// Version 1.1: Add support for Teensy 2.0

#define USB_SERIAL_PRIVATE_INCLUDE
#include "usb_keyboard_rawhid.h"

/**************************************************************************
 *
 *  Configurable Options
 *
 **************************************************************************/

#define STR_MANUFACTURER	L"unspecified"  // TODO
#define STR_PRODUCT		L"ErgoDox ergonomic keyboard with modified firmware"
//
// Mac OS-X and Linux automatically load the correct drivers.  On
// Windows, even though the driver is supplied by Microsoft, an
// INF file is needed to load the driver.  These numbers need to
// match the INF file.
#define VENDOR_ID		0x1d50  // Openmoko, Inc.
#define PRODUCT_ID		0x6028  // ErgoDox ergonomic keyboard


// USB devices are supposed to implment a halt feature, which is
// rarely (if ever) used.  If you comment this line out, the halt
// code will be removed, saving 102 bytes of space (gcc 4.3.0).
// This is not strictly USB compliant, but works with all major
// operating systems.
#define SUPPORT_ENDPOINT_HALT

/* report id */
#define REPORT_ID_SYSTEM    2
#define REPORT_ID_CONSUMER  3

// TODO: What these values are?
// #define RAWHID_USAGE_PAGE	0xFFAB	// recommended: 0xFF00 to 0xFFFF
// #define RAWHID_USAGE		0x0200	// recommended: 0x0100 to 0xFFFF

// These determine the bandwidth that will be allocated
// for your communication.  You do not need to use it
// all, but allocating more than necessary means reserved
// bandwidth is no longer available to other USB devices.
// #define RAWHID_TX_SIZE		64	// transmit packet size
// #define RAWHID_TX_INTERVAL	2	// max # of ms between transmit packets
// #define RAWHID_RX_SIZE		64	// receive packet size
// #define RAWHID_RX_INTERVAL	8	// max # of ms between receive packets


/**************************************************************************
 *
 *  Endpoint Buffer Configuration
 *
 **************************************************************************/
#define ENDPOINT0_SIZE		32

#define KEYBOARD_INTERFACE	0
#define KEYBOARD_ENDPOINT	1
#define KEYBOARD_SIZE		8
#define KEYBOARD_BUFFER		EP_DOUBLE_BUFFER

#define EXTRA_INTERFACE		1
#define EXTRA_ENDPOINT		2
#define EXTRA_SIZE		8
#define EXTRA_BUFFER		EP_DOUBLE_BUFFER

// #define RAWHID_INTERFACE	2
// #define RAWHID_TX_ENDPOINT	3
// #define RAWHID_RX_ENDPOINT	4
// #define RAWHID_TX_BUFFER	EP_DOUBLE_BUFFER
// #define RAWHID_RX_BUFFER	EP_DOUBLE_BUFFER

static const uint8_t PROGMEM endpoint_config_table[] = {
	1, EP_TYPE_INTERRUPT_IN,  EP_SIZE(KEYBOARD_SIZE) | KEYBOARD_BUFFER,
	1, EP_TYPE_INTERRUPT_IN,  EP_SIZE(EXTRA_SIZE)    | EXTRA_BUFFER,    // 4
	// 1, EP_TYPE_INTERRUPT_IN,  EP_SIZE(RAWHID_TX_SIZE) | RAWHID_TX_BUFFER,
	// 1, EP_TYPE_INTERRUPT_OUT,  EP_SIZE(RAWHID_RX_SIZE) | RAWHID_RX_BUFFER,
	0
};


/**************************************************************************
 *
 *  Descriptor Data
 *
 **************************************************************************/

// Descriptors are the data that your computer reads when it auto-detects
// this USB device (called "enumeration" in USB lingo).  The most commonly
// changed items are editable at the top of this file.  Changing things
// in here should only be done by those who've read chapter 9 of the USB
// spec and relevant portions of any USB class specifications!


static const uint8_t PROGMEM device_descriptor[] = {
	18,					// bLength
	1,					// bDescriptorType
	0x00, 0x02,				// bcdUSB
	0,					// bDeviceClass
	0,					// bDeviceSubClass
	0,					// bDeviceProtocol
	ENDPOINT0_SIZE,				// bMaxPacketSize0
	LSB(VENDOR_ID), MSB(VENDOR_ID),		// idVendor
	LSB(PRODUCT_ID), MSB(PRODUCT_ID),	// idProduct
	0x00, 0x01,				// bcdDevice
	1,					// iManufacturer
	2,					// iProduct
	0,					// iSerialNumber
	1					// bNumConfigurations
};


// Keyboard Protocol 1, HID 1.11 spec, Appendix B, page 59-60
static const uint8_t PROGMEM keyboard_hid_report_desc[] = {
        0x05, 0x01,          // Usage Page (Generic Desktop),
        0x09, 0x06,          // Usage (Keyboard),
        0xA1, 0x01,          // Collection (Application),
        0x75, 0x01,          //   Report Size (1),
        0x95, 0x08,          //   Report Count (8),
        0x05, 0x07,          //   Usage Page (Key Codes),
        0x19, 0xE0,          //   Usage Minimum (224),
        0x29, 0xE7,          //   Usage Maximum (231),
        0x15, 0x00,          //   Logical Minimum (0),
        0x25, 0x01,          //   Logical Maximum (1),
        0x81, 0x02,          //   Input (Data, Variable, Absolute), ;Modifier byte
        0x95, 0x01,          //   Report Count (1),
        0x75, 0x08,          //   Report Size (8),
        0x81, 0x03,          //   Input (Constant),                 ;Reserved byte
        0x95, 0x05,          //   Report Count (5),
        0x75, 0x01,          //   Report Size (1),
        0x05, 0x08,          //   Usage Page (LEDs),
        0x19, 0x01,          //   Usage Minimum (1),
        0x29, 0x05,          //   Usage Maximum (5),
        0x91, 0x02,          //   Output (Data, Variable, Absolute), ;LED report
        0x95, 0x01,          //   Report Count (1),
        0x75, 0x03,          //   Report Size (3),
        0x91, 0x03,          //   Output (Constant),                 ;LED report padding
        0x95, 0x06,          //   Report Count (6),
        0x75, 0x08,          //   Report Size (8),
        0x15, 0x00,          //   Logical Minimum (0),
        0x25, 0x68,          //   Logical Maximum(104),
        0x05, 0x07,          //   Usage Page (Key Codes),
        0x19, 0x00,          //   Usage Minimum (0),
        0x29, 0x68,          //   Usage Maximum (104),
        0x81, 0x00,          //   Input (Data, Array),
        0xc0                 // End Collection
};

// audio controls & system controls
// http://www.microsoft.com/whdc/archive/w2kbd.mspx
static const uint8_t PROGMEM extra_hid_report_desc[] = {
    /* consumer */
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, REPORT_ID_CONSUMER,      //   REPORT_ID (3)
    0x15, 0x01,                    //   LOGICAL_MINIMUM (0x1)
    0x26, 0x9c, 0x02,              //   LOGICAL_MAXIMUM (0x29c)
    0x19, 0x01,                    //   USAGE_MINIMUM (0x1)
    0x2a, 0x9c, 0x02,              //   USAGE_MAXIMUM (0x29c)
    0x75, 0x10,                    //   REPORT_SIZE (16)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x00,                    //   INPUT (Data,Array,Abs)
    0xc0,                          // END_COLLECTION
};

// static const uint8_t PROGMEM rawhid_hid_report_desc[] = {
// 	0x06, LSB(RAWHID_USAGE_PAGE), MSB(RAWHID_USAGE_PAGE),
// 	0x0A, LSB(RAWHID_USAGE), MSB(RAWHID_USAGE),
// 	0xA1, 0x01,				// Collection 0x01
// 	0x75, 0x08,				// report size = 8 bits
// 	0x15, 0x00,				// logical minimum = 0
// 	0x26, 0xFF, 0x00,			// logical maximum = 255
// 	0x95, RAWHID_TX_SIZE,			// report count
// 	0x09, 0x01,				// usage
// 	0x81, 0x02,				// Input (array)
// 	0x95, RAWHID_RX_SIZE,			// report count
// 	0x09, 0x02,				// usage
// 	0x91, 0x02,				// Output (array)
// 	0xC0					// end collection
// };

#define KEYBOARD_HID_DESC_NUM           0
#define KEYBOARD_HID_DESC_OFFSET        (9+(9+9+7)*KEYBOARD_HID_DESC_NUM+9)

#define EXTRA_HID_DESC_NUM              (KEYBOARD_HID_DESC_NUM + 1)
#define EXTRA_HID_DESC_OFFSET           (9+(9+9+7)*EXTRA_HID_DESC_NUM+9)

// #define RAWHID_HID_DESC_NUM             (EXTRA_HID_DESC_NUM + 1)
// #define RAWHID_HID_DESC_OFFSET          (9+(9+9+7)*RAWHID_HID_DESC_NUM+9)

#define NUM_INTERFACES                  (EXTRA_HID_DESC_NUM + 1)
#define CONFIG1_DESC_SIZE               (9+(9+9+7)*NUM_INTERFACES+7)
//#define KEYBOARD_HID_DESC_OFFSET (9+9)
static const uint8_t PROGMEM config1_descriptor[CONFIG1_DESC_SIZE] = {
	// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
	9, 					// bLength;
	2,					// bDescriptorType;
	LSB(CONFIG1_DESC_SIZE),			// wTotalLength
	MSB(CONFIG1_DESC_SIZE),
	NUM_INTERFACES,					// bNumInterfaces
	1,					// bConfigurationValue
	0,					// iConfiguration
	0xC0,					// bmAttributes
	50,					// bMaxPower
	// interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
	9,					// bLength
	4,					// bDescriptorType
	KEYBOARD_INTERFACE,			// bInterfaceNumber
	0,					// bAlternateSetting
	1,					// bNumEndpoints
	0x03,					// bInterfaceClass (0x03 = HID)
	0x01,					// bInterfaceSubClass (0x01 = Boot)
	0x01,					// bInterfaceProtocol (0x01 = Keyboard)
	0,					// iInterface
	// HID descriptor, HID 1.11 spec, section 6.2.1
	9,					// bLength
	0x21,					// bDescriptorType
	0x11, 0x01,				// bcdHID
	0,					// bCountryCode
	1,					// bNumDescriptors
	0x22,					// bDescriptorType
	sizeof(keyboard_hid_report_desc),     	// wDescriptorLength
	0,
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	7,					// bLength
	5,					// bDescriptorType
	KEYBOARD_ENDPOINT | 0x80,			// bEndpointAddress
	0x03,					// bmAttributes (0x03=intr)
	KEYBOARD_SIZE, 0,				// wMaxPacketSize
	10,					// bInterval

	// interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
	9,					// bLength
	4,					// bDescriptorType
	EXTRA_INTERFACE,			// bInterfaceNumber
	0,					// bAlternateSetting
	1,					// bNumEndpoints
	0x03,					// bInterfaceClass (0x03 = HID)
	0x00,					// bInterfaceSubClass
	0x00,					// bInterfaceProtocol
	0,					// iInterface
	// HID descriptor, HID 1.11 spec, section 6.2.1
	9,					// bLength
	0x21,					// bDescriptorType
	0x11, 0x01,				// bcdHID
	0,					// bCountryCode
	1,					// bNumDescriptors
	0x22,					// bDescriptorType
	sizeof(extra_hid_report_desc),		// wDescriptorLength
	0,
	// endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	7,					// bLength
	5,					// bDescriptorType
	EXTRA_ENDPOINT | 0x80,			// bEndpointAddress
	0x03,					// bmAttributes (0x03=intr)
	EXTRA_SIZE, 0,				// wMaxPacketSize
	10,					// bInterval

	// interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
	// 9,					// bLength
	// 4,					// bDescriptorType
	// RAWHID_INTERFACE,			// bInterfaceNumber
	// 0,					// bAlternateSetting
	// 2,					// bNumEndpoints
	// 0x03,					// bInterfaceClass (0x03 = HID)
	// 0x00,					// bInterfaceSubClass (0x01 = Boot)
	// 0x00,					// bInterfaceProtocol (0x01 = Keyboard)
	// 0,					// iInterface
	// // HID interface descriptor, HID 1.11 spec, section 6.2.1
	// 9,					// bLength
	// 0x21,					// bDescriptorType
	// 0x11, 0x01,				// bcdHID
	// 0,					// bCountryCode
	// 1,					// bNumDescriptors
	// 0x22,					// bDescriptorType
	// sizeof(rawhid_hid_report_desc),		// wDescriptorLength
	// 0,
	// // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	// 7,					// bLength
	// 5,					// bDescriptorType
	// RAWHID_TX_ENDPOINT | 0x80,		// bEndpointAddress
	// 0x03,					// bmAttributes (0x03=intr)
	// RAWHID_TX_SIZE, 0,			// wMaxPacketSize
	// RAWHID_TX_INTERVAL,			// bInterval
	// // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
	// 7,					// bLength
	// 5,					// bDescriptorType
	// RAWHID_RX_ENDPOINT,			// bEndpointAddress
	// 0x03,					// bmAttributes (0x03=intr)
	// RAWHID_RX_SIZE, 0,			// wMaxPacketSize
	// RAWHID_RX_INTERVAL			// bInterval
};

// If you're desperate for a little extra code memory, these strings
// can be completely removed if iManufacturer, iProduct, iSerialNumber
// in the device desciptor are changed to zeros.
struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	int16_t wString[];
};
static const struct usb_string_descriptor_struct PROGMEM string0 = {
	4,
	3,
	{0x0409}
};
static const struct usb_string_descriptor_struct PROGMEM string1 = {
	sizeof(STR_MANUFACTURER),
	3,
	STR_MANUFACTURER
};
static const struct usb_string_descriptor_struct PROGMEM string2 = {
	sizeof(STR_PRODUCT),
	3,
	STR_PRODUCT
};

// This table defines which descriptor data is sent for each specific
// request from the host (in wValue and wIndex).
static struct descriptor_list_struct {
	uint16_t	wValue;
	uint16_t	wIndex;
	const uint8_t	*addr;
	uint8_t		length;
} const PROGMEM descriptor_list[] = {
        // DEVICE descriptor
	{0x0100, 0x0000, device_descriptor, sizeof(device_descriptor)},
        // CONFIGURATION descriptor
	{0x0200, 0x0000, config1_descriptor, sizeof(config1_descriptor)},
        // HID/REPORT descriptors
	{0x2100, KEYBOARD_INTERFACE, config1_descriptor+KEYBOARD_HID_DESC_OFFSET, 9},
	{0x2200, KEYBOARD_INTERFACE, keyboard_hid_report_desc, sizeof(keyboard_hid_report_desc)},
	    // Extra HID Descriptor
	{0x2100, EXTRA_INTERFACE, config1_descriptor+EXTRA_HID_DESC_OFFSET, 9},
	{0x2200, EXTRA_INTERFACE, extra_hid_report_desc, sizeof(extra_hid_report_desc)},
        // RAWHID HID Descriptors
	// {0x2100, RAWHID_INTERFACE, config1_descriptor+RAWHID_HID_DESC_OFFSET, 9},
	// {0x2200, RAWHID_INTERFACE, rawhid_hid_report_desc, sizeof(rawhid_hid_report_desc)},
        // STRING descriptors
	{0x0300, 0x0000, (const uint8_t *)&string0, 4},
	{0x0301, 0x0409, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
	{0x0302, 0x0409, (const uint8_t *)&string2, sizeof(STR_PRODUCT)}
};
#define NUM_DESC_LIST (sizeof(descriptor_list)/sizeof(struct descriptor_list_struct))


/**************************************************************************
 *
 *  Variables - these are the only non-stack RAM usage
 *
 **************************************************************************/

// zero when we are not configured, non-zero when enumerated
static volatile uint8_t usb_configuration=0;

// which modifier keys are currently pressed
// 1=left ctrl,    2=left shift,   4=left alt,    8=left gui
// 16=right ctrl, 32=right shift, 64=right alt, 128=right gui
uint8_t keyboard_modifier_keys=0;

// which keys are currently pressed, up to 6 keys may be down at once
uint8_t keyboard_keys[6]={0,0,0,0,0,0};

// protocol setting from the host.  We use exactly the same report
// either way, so this variable only stores the setting since we
// are required to be able to report which setting is in use.
static uint8_t keyboard_protocol=1;

// the idle configuration, how often we send the report to the
// host (ms * 4) even when it hasn't changed
static uint8_t keyboard_idle_config=125;

// count until idle timeout
static uint8_t keyboard_idle_count=0;

// 1=num lock, 2=caps lock, 4=scroll lock, 8=compose, 16=kana
volatile uint8_t keyboard_leds=0;

// which consumer key is currently pressed
uint16_t consumer_key;
uint16_t last_consumer_key;

// these are a more reliable timeout than polling the
// frame counter (UDFNUML)
// static volatile uint8_t rx_timeout_count=0;
// static volatile uint8_t tx_timeout_count=0;

/**************************************************************************
 *
 *  Public Functions - these are the API intended for the user
 *
 **************************************************************************/


// initialize USB
void usb_init(void)
{
	HW_CONFIG();
	USB_FREEZE();	// enable USB
	PLL_CONFIG();				// config PLL
        while (!(PLLCSR & (1<<PLOCK))) ;	// wait for PLL lock
        USB_CONFIG();				// start USB clock
        UDCON = 0;				// enable attach resistor
	usb_configuration = 0;
        UDIEN = (1<<EORSTE)|(1<<SOFE);
	sei();
}

// return 0 if the USB is not configured, or the configuration
// number selected by the HOST
uint8_t usb_configured(void)
{
	return usb_configuration;
}


// perform a single keystroke
int8_t usb_keyboard_press(uint8_t key, uint8_t modifier)
{
	int8_t r;

	keyboard_modifier_keys = modifier;
	keyboard_keys[0] = key;
	r = usb_keyboard_send();
	if (r) return r;
	keyboard_modifier_keys = 0;
	keyboard_keys[0] = 0;
	return usb_keyboard_send();
}

// send the contents of keyboard_keys and keyboard_modifier_keys
int8_t usb_keyboard_send(void)
{
	uint8_t i, intr_state, timeout;

	if (!usb_configuration) return -1;
	intr_state = SREG;
	cli();
	UENUM = KEYBOARD_ENDPOINT;
	timeout = UDFNUML + 50;
	while (1) {
		// are we ready to transmit?
		if (UEINTX & (1<<RWAL)) break;
		SREG = intr_state;
		// has the USB gone offline?
		if (!usb_configuration) return -1;
		// have we waited too long?
		if (UDFNUML == timeout) return -1;
		// get ready to try checking again
		intr_state = SREG;
		cli();
		UENUM = KEYBOARD_ENDPOINT;
	}
	UEDATX = keyboard_modifier_keys;
	UEDATX = 0;
	for (i=0; i<6; i++) {
		UEDATX = keyboard_keys[i];
	}
	UEINTX = 0x3A;
	keyboard_idle_count = 0;
	SREG = intr_state;
	return 0;
}

// receive a packet, with timeout
// int8_t usb_rawhid_recv(uint8_t *buffer, uint8_t timeout)
// {
// 	uint8_t intr_state;
// 
// 	// if we're not online (enumerated and configured), error
// 	if (!usb_configuration) return -1;
// 	intr_state = SREG;
// 	cli();
// 	rx_timeout_count = timeout;
// 	UENUM = RAWHID_RX_ENDPOINT;
// 	// wait for data to be available in the FIFO
// 	while (1) {
// 		if (UEINTX & (1<<RWAL)) break;
// 		SREG = intr_state;
// 		if (rx_timeout_count == 0) return 0;
// 		if (!usb_configuration) return -1;
// 		intr_state = SREG;
// 		cli();
// 		UENUM = RAWHID_RX_ENDPOINT;
// 	}
// 	// read bytes from the FIFO
// 	#if (RAWHID_RX_SIZE >= 64)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 63)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 62)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 61)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 60)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 59)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 58)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 57)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 56)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 55)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 54)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 53)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 52)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 51)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 50)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 49)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 48)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 47)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 46)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 45)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 44)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 43)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 42)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 41)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 40)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 39)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 38)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 37)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 36)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 35)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 34)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 33)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 32)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 31)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 30)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 29)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 28)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 27)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 26)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 25)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 24)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 23)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 22)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 21)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 20)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 19)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 18)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 17)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 16)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 15)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 14)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 13)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 12)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 11)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 10)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 9)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 8)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 7)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 6)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 5)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 4)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 3)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 2)
// 	*buffer++ = UEDATX;
// 	#endif
// 	#if (RAWHID_RX_SIZE >= 1)
// 	*buffer++ = UEDATX;
// 	#endif
// 	// release the buffer
// 	UEINTX = 0x6B;
// 	SREG = intr_state;
// 	return RAWHID_RX_SIZE;
// }
// 
// // send a packet, with timeout
// int8_t usb_rawhid_send(const uint8_t *buffer, uint8_t timeout)
// {
// 	uint8_t intr_state;
// 
// 	// if we're not online (enumerated and configured), error
// 	if (!usb_configuration) return -1;
// 	intr_state = SREG;
// 	cli();
// 	tx_timeout_count = timeout;
// 	UENUM = RAWHID_TX_ENDPOINT;
// 	// wait for the FIFO to be ready to accept data
// 	while (1) {
// 		if (UEINTX & (1<<RWAL)) break;
// 		SREG = intr_state;
// 		if (tx_timeout_count == 0) return 0;
// 		if (!usb_configuration) return -1;
// 		intr_state = SREG;
// 		cli();
// 		UENUM = RAWHID_TX_ENDPOINT;
// 	}
// 	// write bytes from the FIFO
// 	#if (RAWHID_TX_SIZE >= 64)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 63)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 62)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 61)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 60)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 59)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 58)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 57)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 56)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 55)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 54)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 53)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 52)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 51)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 50)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 49)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 48)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 47)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 46)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 45)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 44)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 43)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 42)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 41)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 40)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 39)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 38)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 37)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 36)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 35)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 34)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 33)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 32)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 31)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 30)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 29)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 28)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 27)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 26)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 25)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 24)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 23)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 22)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 21)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 20)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 19)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 18)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 17)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 16)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 15)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 14)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 13)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 12)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 11)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 10)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 9)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 8)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 7)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 6)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 5)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 4)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 3)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 2)
// 	UEDATX = *buffer++;
// 	#endif
// 	#if (RAWHID_TX_SIZE >= 1)
// 	UEDATX = *buffer++;
// 	#endif
// 	// transmit it now
// 	UEINTX = 0x3A;
// 	SREG = intr_state;
// 	return RAWHID_TX_SIZE;
// }

/**************************************************************************
 *
 *  Private Functions - not intended for general user consumption....
 *
 **************************************************************************/



// USB Device Interrupt - handle all device-level events
// the transmit buffer flushing is triggered by the start of frame
//
ISR(USB_GEN_vect)
{
	uint8_t intbits, i, t;
	static uint8_t div4=0;

        intbits = UDINT;
        UDINT = 0;
        if (intbits & (1<<EORSTI)) {
		UENUM = 0;
		UECONX = 1;
		UECFG0X = EP_TYPE_CONTROL;
		UECFG1X = EP_SIZE(ENDPOINT0_SIZE) | EP_SINGLE_BUFFER;
		UEIENX = (1<<RXSTPE);
		usb_configuration = 0;
        }
	if ((intbits & (1<<SOFI)) && usb_configuration) {
		if (keyboard_idle_config && (++div4 & 3) == 0) {
			UENUM = KEYBOARD_ENDPOINT;
			if (UEINTX & (1<<RWAL)) {
				keyboard_idle_count++;
				if (keyboard_idle_count == keyboard_idle_config) {
					keyboard_idle_count = 0;
					UEDATX = keyboard_modifier_keys;
					UEDATX = 0;
					for (i=0; i<6; i++) {
						UEDATX = keyboard_keys[i];
					}
					UEINTX = 0x3A;
				}
			}
		}
        // t = rx_timeout_count;
        // if (t) rx_timeout_count = --t;
        // t = tx_timeout_count;
        // if (t) tx_timeout_count = --t;
	}
}



// Misc functions to wait for ready and send/receive packets
static inline void usb_wait_in_ready(void)
{
	while (!(UEINTX & (1<<TXINI))) ;
}
static inline void usb_send_in(void)
{
	UEINTX = ~(1<<TXINI);
}
static inline void usb_wait_receive_out(void)
{
	while (!(UEINTX & (1<<RXOUTI))) ;
}
static inline void usb_ack_out(void)
{
	UEINTX = ~(1<<RXOUTI);
}



// USB Endpoint Interrupt - endpoint 0 is handled here.  The
// other endpoints are manipulated by the user-callable
// functions, and the start-of-frame interrupt.
//
ISR(USB_COM_vect)
{
        uint8_t intbits;
	const uint8_t *list;
        const uint8_t *cfg;
	uint8_t i, n, len, en;
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint16_t desc_val;
	const uint8_t *desc_addr;
	uint8_t	desc_length;

        UENUM = 0;
	intbits = UEINTX;
        if (intbits & (1<<RXSTPI)) {
                bmRequestType = UEDATX;
                bRequest = UEDATX;
                wValue = UEDATX;
                wValue |= (UEDATX << 8);
                wIndex = UEDATX;
                wIndex |= (UEDATX << 8);
                wLength = UEDATX;
                wLength |= (UEDATX << 8);
                UEINTX = ~((1<<RXSTPI) | (1<<RXOUTI) | (1<<TXINI));
                if (bRequest == GET_DESCRIPTOR) {
			list = (const uint8_t *)descriptor_list;
			for (i=0; ; i++) {
				if (i >= NUM_DESC_LIST) {
					UECONX = (1<<STALLRQ)|(1<<EPEN);  //stall
					return;
				}
				desc_val = pgm_read_word(list);
				if (desc_val != wValue) {
					list += sizeof(struct descriptor_list_struct);
					continue;
				}
				list += 2;
				desc_val = pgm_read_word(list);
				if (desc_val != wIndex) {
					list += sizeof(struct descriptor_list_struct)-2;
					continue;
				}
				list += 2;
				desc_addr = (const uint8_t *)pgm_read_word(list);
				list += 2;
				desc_length = pgm_read_byte(list);
				break;
			}
			len = (wLength < 256) ? wLength : 255;
			if (len > desc_length) len = desc_length;
			do {
				// wait for host ready for IN packet
				do {
					i = UEINTX;
				} while (!(i & ((1<<TXINI)|(1<<RXOUTI))));
				if (i & (1<<RXOUTI)) return;	// abort
				// send IN packet
				n = len < ENDPOINT0_SIZE ? len : ENDPOINT0_SIZE;
				for (i = n; i; i--) {
					UEDATX = pgm_read_byte(desc_addr++);
				}
				len -= n;
				usb_send_in();
			} while (len || n == ENDPOINT0_SIZE);
			return;
                }
		if (bRequest == SET_ADDRESS) {
			usb_send_in();
			usb_wait_in_ready();
			UDADDR = wValue | (1<<ADDEN);
			return;
		}
		if (bRequest == SET_CONFIGURATION && bmRequestType == 0) {
			usb_configuration = wValue;
			usb_send_in();
			cfg = endpoint_config_table;
			for (i=1; i<5; i++) {
				UENUM = i;
				en = pgm_read_byte(cfg++);
				UECONX = en;
				if (en) {
					UECFG0X = pgm_read_byte(cfg++);
					UECFG1X = pgm_read_byte(cfg++);
				}
			}
        		UERST = 0x1E;
        		UERST = 0;
			return;
		}
		if (bRequest == GET_CONFIGURATION && bmRequestType == 0x80) {
			usb_wait_in_ready();
			UEDATX = usb_configuration;
			usb_send_in();
			return;
		}

		if (bRequest == GET_STATUS) {
			usb_wait_in_ready();
			i = 0;
			#ifdef SUPPORT_ENDPOINT_HALT
			if (bmRequestType == 0x82) {
				UENUM = wIndex;
				if (UECONX & (1<<STALLRQ)) i = 1;
				UENUM = 0;
			}
			#endif
			UEDATX = i;
			UEDATX = 0;
			usb_send_in();
			return;
		}
		#ifdef SUPPORT_ENDPOINT_HALT
		if ((bRequest == CLEAR_FEATURE || bRequest == SET_FEATURE)
		  && bmRequestType == 0x02 && wValue == 0) {
			i = wIndex & 0x7F;
			if (i >= 1 && i <= MAX_ENDPOINT) {
				usb_send_in();
				UENUM = i;
				if (bRequest == SET_FEATURE) {
					UECONX = (1<<STALLRQ)|(1<<EPEN);
				} else {
					UECONX = (1<<STALLRQC)|(1<<RSTDT)|(1<<EPEN);
					UERST = (1 << i);
					UERST = 0;
				}
				return;
			}
		}
		#endif
		if (wIndex == KEYBOARD_INTERFACE) {
			if (bmRequestType == 0xA1) {
				if (bRequest == HID_GET_REPORT) {
					usb_wait_in_ready();
					UEDATX = keyboard_modifier_keys;
					UEDATX = 0;
					for (i=0; i<6; i++) {
						UEDATX = keyboard_keys[i];
					}
					usb_send_in();
					return;
				}
				if (bRequest == HID_GET_IDLE) {
					usb_wait_in_ready();
					UEDATX = keyboard_idle_config;
					usb_send_in();
					return;
				}
				if (bRequest == HID_GET_PROTOCOL) {
					usb_wait_in_ready();
					UEDATX = keyboard_protocol;
					usb_send_in();
					return;
				}
			}
			if (bmRequestType == 0x21) {
				if (bRequest == HID_SET_REPORT) {
					usb_wait_receive_out();
					keyboard_leds = UEDATX;
					usb_ack_out();
					usb_send_in();
					return;
				}
				if (bRequest == HID_SET_IDLE) {
					keyboard_idle_config = (wValue >> 8);
					keyboard_idle_count = 0;
					usb_send_in();
					return;
				}
				if (bRequest == HID_SET_PROTOCOL) {
					keyboard_protocol = wValue;
					usb_send_in();
					return;
				}
			}
		}
		// if (wIndex == RAWHID_INTERFACE) {
		// 	if (bmRequestType == 0xA1 && bRequest == HID_GET_REPORT) {
		// 		len = RAWHID_TX_SIZE;
		// 		do {
		// 			// wait for host ready for IN packet
		// 			do {
		// 				i = UEINTX;
		// 			} while (!(i & ((1<<TXINI)|(1<<RXOUTI))));
		// 			if (i & (1<<RXOUTI)) return;	// abort
		// 			// send IN packet
		// 			n = len < ENDPOINT0_SIZE ? len : ENDPOINT0_SIZE;
		// 			for (i = n; i; i--) {
		// 				// just send zeros
		// 				UEDATX = 0;
		// 			}
		// 			len -= n;
		// 			usb_send_in();
		// 		} while (len || n == ENDPOINT0_SIZE);
		// 		return;
		// 	}
		// 	if (bmRequestType == 0x21 && bRequest == HID_SET_REPORT) {
		// 		len = RAWHID_RX_SIZE;
		// 		do {
		// 			n = len < ENDPOINT0_SIZE ? len : ENDPOINT0_SIZE;
		// 			usb_wait_receive_out();
		// 			// ignore incoming bytes
		// 			usb_ack_out();
		// 			len -= n;
		// 		} while (len);
		// 		usb_wait_in_ready();
		// 		usb_send_in();
		// 		return;
		// 	}
		// }
	}
	UECONX = (1<<STALLRQ) | (1<<EPEN);	// stall
}

int8_t usb_extra_send(uint8_t report_id, uint16_t data)
{
	uint8_t intr_state, timeout;

	if (!usb_configured()) return -1;
	intr_state = SREG;
	cli();
	UENUM = EXTRA_ENDPOINT;
	timeout = UDFNUML + 50;
	while (1) {
		// are we ready to transmit?
		if (UEINTX & (1<<RWAL)) break;
		SREG = intr_state;
		// has the USB gone offline?
		if (!usb_configured()) return -1;
		// have we waited too long?
		if (UDFNUML == timeout) return -1;
		// get ready to try checking again
		intr_state = SREG;
		cli();
		UENUM = EXTRA_ENDPOINT;
	}

	UEDATX = report_id;
        UEDATX = data&0xFF;
        UEDATX = (data>>8)&0xFF;

	UEINTX = 0x3A;
	SREG = intr_state;
	return 0;
}

int8_t usb_extra_consumer_send()
{
	int result = 0;
	// don't resend the same key repeatedly if held, only send it once.
	if (consumer_key != last_consumer_key) {
		result = usb_extra_send(REPORT_ID_CONSUMER, consumer_key);
		if (result == 0) {
			last_consumer_key = consumer_key;
		}
	}
	return result;
}
