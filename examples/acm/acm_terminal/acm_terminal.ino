#include <cdcacm.h>
#include <usbhub.h>

#include "pgmstrings.h"

// On SAMD boards where the native USB port is also the serial console, use
// Serial1 for the serial console. This applies to all SAMD boards except for
// Arduino Zero and M0 boards.
#if (USB_VID==0x2341 && defined(ARDUINO_SAMD_ZERO)) || (USB_VID==0x2a03 && defined(ARDUINO_SAM_ZERO))
#define SerialDebug SERIAL_PORT_MONITOR
#else
#define SerialDebug Serial1
#endif

class ACMAsyncOper : public CDCAsyncOper
{
public:
    uint8_t OnInit(ACM *pacm);
};

uint8_t ACMAsyncOper::OnInit(ACM *pacm)
{
    uint8_t rcode;
    // Set DTR = 1 RTS=1
    rcode = pacm->SetControlLineState(3);

    if (rcode)
    {
        ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
        return rcode;
    }

    LINE_CODING	lc;
    lc.dwDTERate	= 115200;
    lc.bCharFormat	= 0;
    lc.bParityType	= 0;
    lc.bDataBits	= 8;

    rcode = pacm->SetLineCoding(&lc);

    if (rcode)
        ErrorMessage<uint8_t>(PSTR("SetLineCoding"), rcode);

    return rcode;
}

USBHost     UsbH;
//USBHub     Hub(&UsbH);
ACMAsyncOper  AsyncOper;
ACM           AcmSerial(&UsbH, &AsyncOper);

void setup()
{
  SerialDebug.begin( 115200 );
  SerialDebug.println("Start");

  if (UsbH.Init())
      SerialDebug.println("USB host failed to initialize");

  delay( 200 );
  SerialDebug.println("USB Host init OK");
}

void loop()
{
    UsbH.Task();

    if( AcmSerial.isReady()) {
       uint8_t rcode;

       /* reading the keyboard */
       if(SerialDebug.available()) {
         uint8_t data= SerialDebug.read();
         /* sending to the phone */
         rcode = AcmSerial.SndData(1, &data);
         if (rcode)
            ErrorMessage<uint8_t>(PSTR("SndData"), rcode);
       }//if(SerialDebug.available()...

       delay(50);

        /* reading the phone */
        /* buffer size must be greater or equal to max.packet size */
        /* it it set to 64 (largest possible max.packet size) here, can be tuned down
        for particular endpoint */
        uint8_t  buf[64];
        uint16_t rcvd = 64;
        rcode = AcmSerial.RcvData(&rcvd, buf);
        if (rcode && rcode != USB_ERRORFLOW)
          ErrorMessage<uint8_t>(PSTR("Ret"), rcode);

        if( rcvd ) { //more than zero bytes received
          for(uint16_t i=0; i < rcvd; i++ ) {
            SerialDebug.print((char)buf[i]); //printing on the screen
          }
        }
        delay(10);
    }//if( Usb.getUsbTaskState() == USB_STATE_RUNNING..
}
