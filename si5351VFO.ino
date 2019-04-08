/*
 Name:                si5351VFO.ino
 Author:              James M. Lynes, Jr.(KE4MIQ), jmlynesjr@gmail.com
 Giants:     	      Ashhar Farhan, VU2ESE, Allard Munters, PE1NWL, Jerry Gaffke, KE7ER,
                      Jack Purdum, W8TEE
 Version:             v1.00 based primarily on Allard's v1.26
 Created:             January 18, 2019
 Last Modified:	      January 23, 2019
 Environment	      Arduino Nano w/ATmega328 and the Arduino IDE
 Change Log:	      1/18/19   - Complete refactor and strip down of Allard's file v1.26
                                  as already modified in ZZRX40VFO4.ino
                      1/19/19   - Second round of clean-up
                      1/20/19   - Third round of clean-up, clean compile
                      1/23/19   - Next/final comment fixes and additions
 Description:         Operating code for the Raduino VFO from HFSIGS.COM
 Notes:               40M VFO version for use with the ZZRX-40 DC Receiver from 4SQRP
                      Only VFO A is supported, nothing is saved in EEPROM
                      Modify pin assignments to use with other than an HFSIGS Raduino board
                          3 Nano analog pins are used - I2C(A4(SDA), A5(SCL)), Tuning pot center(A7)
                          6 Nano digital pins are used - D8, D9, D10, D11, D12, D13
                      Modify frequency range to suit your frequency needs

   This source file is under General Public License version 3.

   The Raduino is a small board that includes the Arduino Nano, a 16x2 LCD display and
   an si5351a frequency synthesizer. This board is manufactured by Paradigm Ecomm Pvt Ltd. (India)
   It is the size of a standard 16x2 LCD panel and it has three connectors:

   First, is an 8 pin connector that provides +5v, GND and six analog input pins that can also be
   configured to be used as digital input or output pins. These are referred to as A0,A1,A2,
   A3,A6 and A7 pins. The A4 and A5 pins are missing from this connector as they are used to
   talk to the si5351 over I2C protocol.

    A0     A1   A2   A3    GND    +5V   A6   A7
     8      7    6    5     4       3    2    1     (connector P1 pins)
   BLACK BROWN RED ORANGE YELLOW GREEN BLUE VIOLET  (same color coding as used for resistors)

   Though, this can be assigned anyway, for this application of the Arduino, we will make the following
   assignment:

   A0 (analog) Spare in this minimal VFO
   A1 (analog) Spare in this minimal VFO
   A2 (analog) Spare in this minimal VFO
   A3 (analog) Spare in this minimal VFO
   A4 (already in use for talking to the si5351) I2C
   A5 (already in use for talking to the si5351) I2C
   A6 (analog) Spare in this minimal VFO
   A7 (analog) Tuning Pot connected to the center pin of a good quality 100K
                   or 10K linear potentiometer with the two other ends
                   connected to ground and +5v lines available on the connector.

   Second, is a 16 pin LCD connector. This connector is meant specifically for the standard 16x2
   LCD display in 4 bit mode. The 4 bit mode requires 4 data lines and two control lines to work:
   Lines used are : RESET, ENABLE,  D4,  D5,  D6,  D7 - LCD module pins
                      D8     D9    D10  D11  D12  D13 - Nano pins

   Third, the set of 16 pins on the bottom connector P3 have the three clock outputs
       and the digital lines to control the rig.
   This assignment is as follows :
      Pin   1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16  (connector P3)
         +12V +12V CLK2  GND  GND CLK1  GND  GND  CLK0  GND  D2   D3   D4   D5   D6   D7

    D2 (digital) Spare in this minimal VFO
    D3 (digital) Spare in this minimal VFO
    D4 (digital) Spare in this minimal VFO
    D5 (digital) Spare in this minimal VFO
    D6 (digital) Spare in this minimal VFO
    D7 (digital) Spare in this minimal VFO
*/

// *** Misc parameters ***

// tuning range parameters
#define MIN_FREQ 7000000UL        // absolute minimum tuning frequency in Hz (C)
#define MAX_FREQ 7300000UL        // absolute maximum tuning frequency in Hz (C)
#define START_FREQ 7200000UL      // Startup frequency                       (C)
#define TUNING_POT_SPAN 50        // tuning pot span in kHz [accepted range 10-500 kHz] (C)
                                  // recommended pot span for a 1-turn pot: 50kHz,
                                  // for a 10-turn pot: 100 to 200kHz
                                  // recommended pot span when radio is used mainly for CW: 10 to 25 kHz
#define CAL_VALUE 1944            // VFO calibration value - correct for 1.6 KHz clock error (C)
#define ANALOG_TUNING (A7)        // Pot center pin

// *** si5351 driver parameters
#define BB0(x) ((uint8_t)x)                             // Bust int32 into Bytes
#define BB1(x) ((uint8_t)(x>>8))
#define BB2(x) ((uint8_t)(x>>16))

#define SI5351BX_ADDR 0x60                              // I2C address of si5351   (typical)
#define SI5351BX_XTALPF 2                               // 1:6pf  2:8pf  3:10pf

// If using 27mhz crystal, set XTAL=27000000, MSA=33.  Then vco=891mhz
#define SI5351BX_XTAL 25000000                          // Crystal freq in Hz
#define SI5351BX_MSA  35                                // VCOA is at 25mhz*35 = 875mhz

// User program may have reason to poke new values into these 3 RAM variables
uint32_t si5351bx_vcoa = (SI5351BX_XTAL*SI5351BX_MSA);  // 25mhzXtal calibrate
uint8_t  si5351bx_rdiv = 0;                             // 0-7, CLK pin sees fout/(2**rdiv)
uint8_t  si5351bx_drive[3] = {0, 0, 0};                 // 0=2ma 1=4ma 2=6ma 3=8ma for CLK 0,1,2
uint8_t  si5351bx_clken = 0xFF;                         // Private, all CLK output drivers off

struct userparameters {
  byte raduino_version;                                 // version identifier
  int cal = CAL_VALUE;                                  // VFO calibration value
  unsigned int POT_SPAN = TUNING_POT_SPAN;              // tuning pot span (kHz)
  unsigned long vfoA = START_FREQ;                      // starting frequency of VFO A
  unsigned long LOWEST_FREQ = MIN_FREQ;                 // absolute minimum dial frequency (Hz)
  unsigned long HIGHEST_FREQ = MAX_FREQ;                // absolute maximum dial frequency (Hz)
};

// we can access each of the variables inside the above struct by "u.variablename"
struct userparameters u;                           // Create structure u


// Below are the libraries to be included for building the Raduino
#include <Wire.h>                                  // The Wire.h library is used to talk
                                                   //     to the si5351 via I2C
#include <LiquidCrystal.h>                         // The LiquidCrystal.h library is used to talk
                                                   //     to the LCD display
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);           // Create an instance of the LCD object called lcd
                                                   // RST, EN, D4, D5, D6, D7

// Working memory allocations
char c[17], b[10], printBuff[2][17];               // Allocate the display/print buffers
char callsign[17] = "KE4MIQ v1.00    ";            // Call sign to display on line 1
char banner[17] = "si5351 VFO v1.00";              // Start-up banner message on Line 1

//   The Raduino needs to keep track of the current state of the transceiver.
//       These are a few variables that do it

unsigned long vfoA;
unsigned long baseTune = START_FREQ;
int old_knob = 0;                                 // Last value of the tuning pot
unsigned long frequency;                          // The si5351 generated frequency


/** Tuning Mechanism of the Raduino
    We use a linear pot that has two ends connected to +5 and the ground. the middle wiper
    is connected to the ANALOG_TUNNING pin. Depending upon the position of the wiper, the
    reading can be anywhere from 0 to 1023.
    If we want to use a multi-turn potentiometer covering 500 kHz and a step
    size of 50 Hz we need 10,000 steps which is about 10x more than the steps that the ADC
    provides. Arduino's ADC has 10 bits which results in 1024 steps only.
    We can artificially expand the number of steps by a factor 10 by oversampling 100 times.
    As a result we get 10240 steps.
    The tuning control works in steps of 50Hz each for every increment between 0 and 10000.
    Hence turning the pot fully from one end to the other will cover 50 x 10000 = 500 KHz.
    But if we use the standard 1-turn pot, then a tuning range of 500 kHz would be too much.
    (tuning would become very touchy). Tuning beyond the 50 KHz window
    limits is still possible by the fast 'scan-up' and 'scan-down' mode at the end of the pot.
    At the two ends, that is, the tuning starts stepping up or down in 10 KHz steps.
    To stop the scanning the pot is moved back from the edge.
*/

/* --------------------- setup ------------------------------------------------------------

   setup() is called on boot up and runs only once
   It initiliaizes the si5351 and sets various variables to initial state
*/

void setup() {
  u.raduino_version = 1;

  analogReference(DEFAULT);

  lcd.begin(16, 2);                                  // Configure the LCD Display

//  Serial.begin(9600);                              // Start serial, used to send messages to
                                                     // the IDE serial monitor for debugging
  printLine(0, (char *) banner);                     // Print banner msg
  printLine(1, (char *) "");                         // Clear bottom line of display
  delay(2000);                                       // Display the banner msg for 2 secs

  si5351bx_init();                                   // initialize the si5351
  si5351bx_vcoa = (SI5351BX_XTAL * SI5351BX_MSA) + u.cal * 100L; // apply the calibration
                                                                 //     correction factor

  vfoA = u.vfoA;                                     // Set initial VFO frequency
  frequency = vfoA;

  shiftBase();                                       // align the current knob position
                                                     //     with the current frequency

  delay(1000);                                       // Allow 1 sec for si5351 to startup

  printLine(1, (char *) callsign);                   // Display call sign on line 1
}


//---------------------------------- loop ------------------------------------------------------
void loop() {                                       // Runs until processor is reset
    doTuning();
    delay(60);                                      // Adjust for tuning desired responsiveness
}

//  si5315 Driver - *************  Jerry Gaffke, KE7ER   ***********************
/*
    The main chip which generates up to three oscillators of various frequencies in the
    Raduino is the si5351a. To learn more about si5351a you can download the datasheet
    from www.silabs.com although, strictly speaking it is not a requirment to understand this code.

    We no longer use the standard si5351 library because of its huge overhead due to many unused
    features consuming a lot of program space. Instead of depending on an external library we now use
    Jerry Gaffke's, KE7ER, lightweight standalone mimimalist "si5351bx" routines (see further down the
    code).

   A minimalist standalone set of Si5351 routines.
   VCOA is fixed at 875mhz fo a 25MHz clock, VCOB not used.
   The output msynth dividers are used to generate 3 independent clocks
   with 1hz resolution to any frequency between 4khz and 109mhz.

   Usage:
   Call si5351bx_init() once at startup with no args;
   Call si5351bx_setfreq(clknum, freq) each time one of the
   three output CLK pins is to be updated to a new frequency.
   A freq of 0 serves to shut down that output clock.

   The global variable si5351bx_vcoa starts out equal to the nominal VCOA
   frequency of 25mhz*35 = 875000000 Hz.  To correct for 25mhz crystal errors,
   the user can adjust this value.  The vco frequency will not change but
   the number used for the (a+b/c) output msynth calculations is affected.
   Example:  We call for a 5mhz signal, but it measures to be 5.001mhz.
   So the actual vcoa frequency is 875mhz*5.001/5.000 = 875175000 Hz,
   To correct for this error:     si5351bx_vcoa=875175000;

   Most users will never need to generate clocks below 500khz.
   But it is possible to do so by loading a value between 0 and 7 into
   the global variable si5351bx_rdiv, be sure to return it to a value of 0
   before setting some other CLK output pin.  The affected clock will be
   divided down by a power of two defined by  2**si5351_rdiv
   A value of zero gives a divide factor of 1, a value of 7 divides by 128.
 This lightweight method is a reasonable compromise for a seldom used feature.
*/

void si5351bx_init() {                         // Call once at power-up, start PLLA
  uint8_t reg;  uint32_t msxp1;
  Wire.begin();
  i2cWrite(149, 0);                            // SpreadSpectrum off
  i2cWrite(3, si5351bx_clken);                 // Disable all CLK output drivers
  i2cWrite(183, SI5351BX_XTALPF << 6 | 0x12);  // Set 25mhz crystal load capacitance
  msxp1 = 128 * SI5351BX_MSA - 512;            // and msxp2=0, msxp3=1, not fractional
  uint8_t  vals[8] = {0, 1, BB2(msxp1), BB1(msxp1), BB0(msxp1), 0, 0, 0};
  i2cWriten(26, vals, 8);                      // Write to 8 PLLA msynth regs
  i2cWrite(177, 0x20);                         // Reset PLLA  (0x80 resets PLLB)
  // for (reg=16; reg<=23; reg++) i2cWrite(reg, 0x80);    // Powerdown CLK's
  // i2cWrite(187, 0);                         // No fannout of clkin, xtal, ms0, ms4
}

void si5351bx_setfreq(uint8_t clknum, uint32_t fout) {  // Set a CLK to fout Hz
  uint32_t  msa, msb, msc, msxp1, msxp2, msxp3p2top;
  if ((fout < 500000) || (fout > 109000000))   // If clock freq out of range(50KHZ to 109MHz)
    si5351bx_clken |= 1 << clknum;             //  shut down the clock
  else {
    msa = si5351bx_vcoa / fout;                // Integer part of vco/fout
    msb = si5351bx_vcoa % fout;                // Fractional part of vco/fout
    msc = fout;                                // Divide by 2 till fits in reg
    while (msc & 0xfff00000) {
      msb = msb >> 1;
      msc = msc >> 1;
    }
    msxp1 = (128 * msa + 128 * msb / msc - 512) | (((uint32_t)si5351bx_rdiv) << 20);
    msxp2 = 128 * msb - 128 * msb / msc * msc; // msxp3 == msc;
    msxp3p2top = (((msc & 0x0F0000) << 4) | msxp2);     // 2 top nibbles
    uint8_t vals[8] = { BB1(msc), BB0(msc), BB2(msxp1), BB1(msxp1),
                        BB0(msxp1), BB2(msxp3p2top), BB1(msxp2), BB0(msxp2)
                      };
    i2cWriten(42 + (clknum * 8), vals, 8);     // Write to 8 msynth regs
    i2cWrite(16 + clknum, 0x0C | si5351bx_drive[clknum]); // use local msynth
    si5351bx_clken &= ~(1 << clknum);          // Clear bit to enable clock
  }
  i2cWrite(3, si5351bx_clken);                 // Enable/disable clock
}

void i2cWrite(uint8_t reg, uint8_t val) {      // write byte to reg via i2c
  Wire.beginTransmission(SI5351BX_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void i2cWriten(uint8_t reg, uint8_t *vals, uint8_t vcnt) {  // write byte array via I2C
  Wire.beginTransmission(SI5351BX_ADDR);
  Wire.write(reg);
  while (vcnt--) Wire.write(*vals++);
  Wire.endTransmission();
}


/*
   Display Routine printLine()
   This display routine prints a line of characters to the upper or lower line of the 16x2 display
   linenmbr = 0 is the upper line
   linenmbr = 1 is the lower line
   strcmp() returns false if the strings match
*/

void printLine(char linenmbr, char *c) {
  if (strcmp(c, printBuff[linenmbr])) {     // only refresh the display when there was a change
    lcd.setCursor(0, linenmbr);             // place the cursor at the beginning of the selected line
    lcd.print(c);                           // Print the string
    strcpy(printBuff[linenmbr], c);         // move c[] to printBuff

    for (byte i = strlen(c); i < 16; i++) { // c may be shorter than 16 characters add white spaces
      lcd.print(' ');                       //     until the end of the 16 characters line is reached
    }
  }
}

/*
   updateDisplay builds the first line as per current state of the radio
       Array b[] holds the text version of the current generated frequency
       Array c[] holds the 16 character line of text to be displayed
       b[] is copied into c[] along with punctuation characters
       To display a frequency different from the actual si5351 generated frequency
           add an offset or multiplier to "frequency" below
           ex: (frequency + Offset) or (frequency * multplier)
   tks Jack Purdum W8TEE replaced fsprint commmands by str commands for code size reduction
*/

void updateDisplay() {

  memset(c, 0, sizeof(c));                     // Zero out the c[] array
  memset(b, 0, sizeof(b));                     // Zero out the b[] array

  ultoa(frequency, b, DEC);                    // Convert frequency to text
  strcpy(c, "A ");                             // Indicate VFO A
  c[2] = b[0];                                 // ex: A 7.030.000 
  strcat(c, "."); 
  strncat(c, &b[1], 3);
  strcat(c, ".");
  strncat(c, &b[4], 3);
  printLine(0, c);                             // Display c[] on top line of LCD
}


void setFrequency() {                          // Set the si5351 clock 2 frequency

    frequency = (frequency / 100) * 100;       // Force to 100Hz steps to reduce pot jitter(C)
    si5351bx_setfreq(2, frequency);            // Update clock 2
    updateDisplay();                           // Update LCD display
}

// Read the position of the tuning knob at high precision (Allard, PE1NWL)
int knob_position() {
  unsigned long knob = 0;
  // the knob value normally ranges from 0 through 1023 (10 bit ADC)
  // in order to increase the precision by a factor 10, we need 10^2 = 100x oversampling
  for (byte i = 0; i < 100; i++) {
    knob = knob + analogRead(ANALOG_TUNING);   // Accumulate 100 readings from the ADC
  }
  knob = (knob + 5L) / 10L; // take the average of the 100 readings and multiply the result by 10
  //now the knob value ranges from 0 through 10,230 (10x more precision)
  knob = knob * 10000L / 10230L; // scale the knob range down to 0-10,000
  return (int)knob;
}


/*
   Function to align the current knob position with the current frequency.
   We need to apply some offset so that the new frequency
   corresponds with the current knob position.
   This function reads the current knob position, then it shifts the baseTune value up or down
   so that the new frequency matches again with the current knob position.
*/

void shiftBase() {
  setFrequency();
  unsigned long knob = knob_position(); // get the current tuning knob position
  baseTune = frequency - (knob * (unsigned long)u.POT_SPAN / 10UL);
}

/*
   The Tuning mechansim of the Raduino works in a very innovative way.
   It uses a tuning potentiometer. The tuning potentiometer sets a voltage
   between 0 and 5 volts at ANALOG_TUNING pin of the control connector.
   This is read as a value between 0 and 1023. By 100x oversampling this
   range is expanded by a factor 10. Hence, the tuning pot gives you 10,000
   steps from one end to the other end of its rotation. Each step is 50 Hz,
   thus giving maximum 500 Khz of tuning range. The tuning range is scaled down
   depending on the POT_SPAN value.The standard tuning range 
   (for the standard 1-turn pot) is 50 Khz. But it is also possible to use a 10-turn pot
   to tune accross the entire 40m band. When the potentiometer is moved to either
   end of the range, the frequency starts automatically moving up or down in 10 Khz
   increments, so it is still possible to tune beyond the range set by POT_SPAN.
   Tuning is forced to remain between MIN_FREQ and MAX_FREQ.
*/

void doTuning() {
  int knob = analogRead(ANALOG_TUNING) * 100000 / 10230; // get the current tuning knob position

  if (abs(knob - old_knob) < 20)               // ??? 20 count pot deadband ???

  knob = knob_position();                      // get the precise tuning knob position
                                               // the knob is fully on the low end, do
                                               // fast tune: move down by 10 Khz and wait for 500 msec
                                               // if the POT_SPAN is very small (less than 25 kHz)
                                               // then use 1 kHz steps instead

  if (knob == 0) {                             // Minimum pot position, fast tune down
    if (frequency > u.LOWEST_FREQ) {
      if (u.POT_SPAN < 25) baseTune = baseTune - 1000UL; // fast tune down in 1 kHz steps
      else
          baseTune = baseTune - 10000UL;       // fast tune down in 10 kHz steps
          frequency = baseTune + (unsigned long)knob * (unsigned long)u.POT_SPAN / 10UL;
          printLine(1, (char *)"<<<<<<<");     // tks Paul KC8WBK
          delay(500);
    }
    if (frequency <= u.LOWEST_FREQ) baseTune = frequency = u.LOWEST_FREQ; // Latch frequency at low limit
    setFrequency();
    old_knob = 0;
  }

  // the knob is full on the high end, do fast tune: move up by 10 Khz and wait for 500 msec
  // if the POT_SPAN is very small (less than 25 kHz) then use 1 kHz steps instead

  else if (knob == 10000) {                              // Maximum pot position, fast tune up
    if (frequency < u.HIGHEST_FREQ) {
      if (u.POT_SPAN < 25) baseTune = baseTune + 1000UL; // fast tune up in 1 kHz steps
      else
      baseTune = baseTune + 10000UL;                     // fast tune up in 10 kHz steps
      frequency = baseTune + (unsigned long)knob * (unsigned long)u.POT_SPAN / 10UL;
      printLine(1, (char *)"         >>>>>>>");          // tks Paul KC8WBK
      delay(500);
    }
    if (frequency >= u.HIGHEST_FREQ) {
      baseTune = u.HIGHEST_FREQ - (u.POT_SPAN * 1000UL); // Latch frequency at high limit
      frequency = u.HIGHEST_FREQ;
    }
    setFrequency();
    old_knob = 10000;
  }

  // the tuning knob is at neither extremity, tune the signals normally
  else {
    if (abs(knob - old_knob) > 4) {                    // improved "flutter fix":
                                                       //     only change frequency when the current
                                                       //     knob position is more than 4 steps
                                                       //     away from the previous position
      knob = (knob + old_knob) / 2;                    //     tune to the midpoint between current
      old_knob = knob;                                 //     and previous knob reading
      frequency = constrain(baseTune + (unsigned long)knob * (unsigned long)u.POT_SPAN / 10UL, u.LOWEST_FREQ, u.HIGHEST_FREQ);
      setFrequency();
      printLine(1, (char *) callsign);                 // Clear <<< >>> from bottom line, restore call sign
    }
  }
}

