#include <avr/io.h>
#include <util/delay.h>
#include <Log.h>
#include <Knx.h>

int main()
{

  // Configure clock to 32MHz
  OSC.CTRL |= OSC_RC32MEN_bm | OSC_RC32KEN_bm;  /* Enable the internal 32MHz & 32KHz oscillators */
  while(!(OSC.STATUS & OSC_RC32KRDY_bm));       /* Wait for 32Khz oscillator to stabilize */
  while(!(OSC.STATUS & OSC_RC32MRDY_bm));       /* Wait for 32MHz oscillator to stabilize */
  DFLLRC32M.CTRL = DFLL_ENABLE_bm ;             /* Enable DFLL - defaults to calibrate against internal 32Khz clock */
  CCP = CCP_IOREG_gc;                           /* Disable register security for clock update */
  CLK.CTRL = CLK_SCLKSEL_RC32M_gc;              /* Switch to 32MHz clock */
  OSC.CTRL &= ~OSC_RC2MEN_bm;                   /* Disable 2Mhz oscillator */


  Log::init();
  Log::log("\n\r----------------------------------------------------------\n\r");
  Log::log("start program\n\r"); //%d", start);

  PORTB.DIRSET |= PIN0_bm ; // Set pin 0 to be output.

  // global enable interrupts
  sei();

  // knx io
  Knx::init();




  int counter =0;
  while(1)
  { // loop forever
    if (Knx::getPacketsReceivedAmount() > 0)
    {
      // next knx packet
      KnxPacket knxPacket = Knx::getNextPacketBlocking();

      Log::log("\n\r\n\r---- Nr.%d\n\r", counter);
      knxPacket.print();
      //free(&knxPacket);
      counter++;
    }

    /*
    _delay_ms(300);
    KnxPacket packetSend;
    packetSend.setSourceAddr(2,1,17);
    packetSend.setDestinationGroupAddr(0,0,11);
    packetSend.setDataValueSwitch(true);
    //packetSend.send();


    _delay_ms(300);
    KnxPacket packetSend2;
    packetSend2.setSourceAddr(2,1,17);
    packetSend2.setDestinationGroupAddr(0,0,11);
    packetSend2.setDataValueSwitch(false);
    //packetSend2.send();
  */
  }
}