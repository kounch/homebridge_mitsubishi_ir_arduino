/*
 * IRremote
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.htm http://arcfn.com
 * Edited by Mitra to add new controller SANYO
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 *
 * JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
 * LG added by Darryl Smith (based on the JVC protocol)
 *
 * Mitsubishi HVAC protocol added by Vincent Cruvellier.
 * Mitsubishi HVAC W001CP R61Y23304 protocol added by bt4wang.
 */

#ifndef IRremote_h
#define IRremote_h

// The following are compile-time library options.
// If you change them, recompile the library.
// If DEBUG is defined, a lot of debugging output will be printed during decoding.
// TEST must be defined for the IRtest unittests to work.  It will make some
// methods virtual, which will be slightly slower, which is why it is optional.
// #define DEBUG
// #define TEST

// Results returned from the decoder
class decode_results {
  public:
    int decode_type; // NEC, SONY, RC5, UNKNOWN
    union { // This is used for decoding Panasonic and Sharp data
      unsigned int panasonicAddress;
      unsigned int sharpAddress;
    };
    unsigned long value; // Decoded value
    int bits; // Number of bits in decoded value
    volatile unsigned int *rawbuf; // Raw intervals in .5 us ticks
    int rawlen; // Number of records in rawbuf.
};

// Values for decode_type
#define NEC 1
#define SONY 2
#define RC5 3
#define RC6 4
#define DISH 5
#define SHARP 6
#define PANASONIC 7
#define JVC 8
#define SANYO 9
#define MITSUBISHI 10
#define SAMSUNG 11
#define LG 12
#define UNKNOWN -1

// Decoded value for NEC when a repeat code is received
#define REPEAT 0xffffffff

// main class for receiving IR
class IRrecv
{
  public:
    IRrecv(int recvpin);
    void blink13(int blinkflag);
    int decode(decode_results *results);
    void enableIRIn();
    void resume();
  private:
    // These are called by decode
    int getRClevel(decode_results *results, int *offset, int *used, int t1);
    long decodeNEC(decode_results *results);
    long decodeSony(decode_results *results);
    long decodeSanyo(decode_results *results);
    long decodeMitsubishi(decode_results *results);
    long decodeRC5(decode_results *results);
    long decodeRC6(decode_results *results);
    long decodePanasonic(decode_results *results);
    long decodeLG(decode_results *results);
    long decodeJVC(decode_results *results);
    long decodeSAMSUNG(decode_results *results);
    long decodeHash(decode_results *results);
    int compare(unsigned int oldval, unsigned int newval);

}
;

// Only used for testing; can remove virtual for shorter code
#ifdef TEST
#define VIRTUAL virtual
#else
#define VIRTUAL
#endif

typedef enum HvacMode {
  HVAC_HOT,
  HVAC_COLD,
  HVAC_DRY,
  HVAC_FAN, // used for Panasonic only
  HVAC_AUTO
} HvacMode_t; // HVAC  MODE

typedef enum HvacFanMode {
  FAN_SPEED_1,
  FAN_SPEED_2,
  FAN_SPEED_3,
  FAN_SPEED_4,
  FAN_SPEED_5,
  FAN_SPEED_AUTO,
  FAN_SPEED_SILENT
} HvacFanMode_;  // HVAC  FAN MODE

typedef enum HvacVanneMode {
  VANNE_AUTO,
  VANNE_H1,
  VANNE_H2,
  VANNE_H3,
  VANNE_H4,
  VANNE_H5,
  VANNE_AUTO_MOVE
} HvacVanneMode_;  // HVAC  VANNE MODE

typedef enum HvacWideVanneMode {
  WIDE_LEFT_END,
  WIDE_LEFT,
  WIDE_MIDDLE,
  WIDE_RIGHT,
  WIDE_RIGHT_END,
  WIDE_SWING
} HvacWideVanneMode_t;  // HVAC  WIDE VANNE MODE

typedef enum HvacAreaMode {
  AREA_SWING,
  AREA_LEFT,
  AREA_AUTO,
  AREA_RIGHT
} HvacAreaMode_t;  // HVAC  WIDE VANNE MODE

typedef enum HvacProfileMode {
  NORMAL,
  QUIET,
  BOOST
} HvacProfileMode_t;  // HVAC PANASONIC OPTION MODE


class IRsend
{

  public:
    IRsend() {}
    void sendNEC(unsigned long data, int nbits);
    void sendSony(unsigned long data, int nbits);
    // Neither Sanyo nor Mitsubishi send is implemented yet
    //  void sendSanyo(unsigned long data, int nbits);
    //  void sendMitsubishi(unsigned long data, int nbits);
    void sendRaw(unsigned int buf[], int len, int hz);
    void sendRC5(unsigned long data, int nbits);
    void sendRC6(unsigned long data, int nbits);
    void sendDISH(unsigned long data, int nbits);
    void sendSharp(unsigned int address, unsigned int command);
    void sendSharpRaw(unsigned long data, int nbits);
    void sendPanasonic(unsigned int address, unsigned long data);
    void sendHvacMitsubishi(
      HvacMode                  HVAC_Mode,           // Example HVAC_HOT  HvacMitsubishiMode
      int                       HVAC_Temp,           // Example 21  (°c)
      HvacFanMode               HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacMitsubishiFanMode
      HvacVanneMode             HVAC_VanneMode,      // Example VANNE_AUTO_MOVE  HvacMitsubishiVanneMode
      int                       OFF                  // Example false
    );
    // Add support for W001CP R61Y23304 Remote Controller
    void sendHvacMitsubishi_W001CP(
      HvacMode                  HVAC_Mode,           // Example HVAC_HOT.         HvacMitsubishiMode
                                                     // This type support HVAC_HOT,HVAC_COLD,HVAC_DRY,HVAC_FAN,HVAC_AUTO.
      int                       HVAC_Temp,           // Example 21  (°c).
                                                     // This type support 17~28 in HVAC_HOT mode, 19~30 in HVAC_COLD and HVAC_DRY mode.
      HvacFanMode               HVAC_FanMode,        // Example FAN_SPEED_AUTO.   HvacMitsubishiFanMode
                                                     // This type support FAN_SPEED_1,FAN_SPEED_2,FAN_SPEED_3,FAN_SPEED_4.
      HvacVanneMode             HVAC_VanneMode,      // Example VANNE_AUTO_MOVE.  HvacMitsubishiVanneMode
                                                     // This type support support VANNE_AUTO,VANNE_H1,VANNE_H2,VANNE_H3,VANNE_H4.
      int                       OFF                  // Example false
    );
    void sendHvacMitsubishiFD(
      HvacMode                  HVAC_Mode,           // Example HVAC_HOT  HvacMitsubishiMode
      int                       HVAC_Temp,           // Example 21  (°c)
      HvacFanMode               HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacMitsubishiFanMode
      HvacVanneMode             HVAC_VanneMode,      // Example VANNE_AUTO_MOVE  HvacMitsubishiVanneMode
      HvacAreaMode              HVAC_AreaMode,       // Example AREA_AUTO
      HvacWideVanneMode         HVAC_WideMode,       // Example WIDE_MIDDLE
      int                       HVAC_PLASMA,          // Example true to Turn ON PLASMA Function
      int                       HVAC_CLEAN_MODE,      // Example false 
      int                       HVAC_ISEE,            // Example False
      int                       OFF                   // Example false to Turn ON HVAC / true to request to turn off
    );
    void sendHvacPanasonic(
      HvacMode                  HVAC_Mode,           // Example HVAC_HOT  HvacPanasonicMode
      int                       HVAC_Temp,           // Example 21  (°c)
      HvacFanMode               HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacPanasonicFanMode
      HvacVanneMode             HVAC_VanneMode,      // Example VANNE_AUTO_MOVE  HvacPanasonicVanneMode
      HvacProfileMode           HVAC_ProfileMode,    // Example QUIET HvacProfileMode
      int                       HVAC_SWITCH          // Example false
    );
    void sendHvacToshiba(
      HvacMode                HVAC_Mode,           // Example HVAC_HOT
      int                     HVAC_Temp,           // Example 21  (°c)
      HvacFanMode             HVAC_FanMode,        // Example FAN_SPEED_AUTO
      int                     OFF                  // Example false
    );
    
    void sendJVC(unsigned long data, int nbits, int repeat); // *Note instead of sending the REPEAT constant if you want the JVC repeat signal sent, send the original code value and change the repeat argument from 0 to 1. JVC protocol repeats by skipping the header NOT by sending a separate code value like NEC does.
    // private:
    void sendSAMSUNG(unsigned long data, int nbits);
    void enableIROut(int khz);
    VIRTUAL void mark(int usec);
    VIRTUAL void space(int usec);
}
;

// Some useful constants

#define USECPERTICK 50  // microseconds per clock interrupt tick
#define RAWBUF 100 // Length of raw duration buffer

// Marks tend to be 100us too long, and spaces 100us too short
// when received due to sensor lag.
#define MARK_EXCESS 100

#endif
