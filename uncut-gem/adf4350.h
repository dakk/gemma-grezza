/* 
   ADF4350.h - ADF4350 PLL Communication Library
   Created by Neal Pisenti, 2013.
   JQI - Strontium - UMD

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   aunsigned long with this program.  If not, see <http://www.gnu.org/licenses/>.


 */

#ifndef ADF4350_h
#define ADF4350_h

#include "Arduino.h"


#define SPI_MISO MISO //19 ?
#define SPI_MOSI MOSI //23 
#define SPI_SCLK SCK //18
#define SPI_SS   SS //5

#define MAXVAL 1500

#if !defined(CONFIG_IDF_TARGET_ESP32)
#define VSPI FSPI
#endif

static const int spiClk = 100000;  // .1 MHz

class ADF4350
{
    public: 
        // Constructor function. 
        // Creates PLL object, with given LE pin, ADC pin
        ADF4350(byte, byte);

        // Initialize with initial frequency, refClk (defaults to 10Mhz); 
        void init(int, int);


        // powers down the PLL/VCO
        void powerDown(bool);
        void setRfPower(int);
        void setAuxPower(int);
        void auxEnable(bool);
        void rfEnable(bool);

        // Gets current frequency
        int getFreq();

        // Sets frequency
        void setFreq(int);

        void setInt(int);

        void setFeedbackType(bool);

        void update();

        void send_2870(); // added to check stuff
        void send_sweep(); // sweeps the ADF435x from 2752MHz in 128 steps of 2MHz each...
        int send_sweep_step(int);

        unsigned long _phase;
        unsigned long _freq, _int, _divider, _refClk, _auxPower, _rfPower;
        SPIClass *spi = NULL;
        
    private:
        // Instance variables that hold pinout mapping
        byte _LEPin;
        byte _ADCpin;

        // Instance variables for ...
        bool _powerdown, _auxEnabled, _rfEnabled, _feedbackType;

        // register values
        byte _r0[4], _r1[4], _r2[4], _r3[4], _r4[4], _r5[4];



        // function to write data to register.
        void setR0();
        void setR1();
        void setR2();
        void setR3();
        void setR4();
        void setR5();
        void writeRegister(byte[4]);
        int  get_avg_ADC();
};
 

#endif
