//New receiving method. This method checks the Rx Fifo for any data it contains.
//It allows you to do several things in a loop.
//In addition, the gdo0 and gdo2 pin are not required.
//https://github.com/LSatan/SmartRC-CC1101-Driver-Lib
//by Little_S@tan
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define PD_MIN_PULSES 16
#define PD_MAX_PULSES 1200
#define MINIMUM_PULSE_LENGTH 50
#define MINIMUM_SIGNAL_LENGTH 40000
#define MINRSSI -65
#define RECEIVER_BUFFER_SIZE 2
#ifndef ONBOARD_LED
#define ONBOARD_LED 2
#endif

static bool receiveMode = false;
static unsigned long signalStart = micros();
static unsigned long _lastChange = micros();
static int signalRssi = 0;
int currentRssi = 0;
int minimumRssi = MINRSSI;
volatile uint16_t _nrpulses = 0;

bool _enabledReceiver = false;

/// Data for a compact representation of generic pulse train.
typedef struct pulse_data
{
  //    uint64_t offset;            ///< Offset to first pulse in number of samples from start of stream.
  uint32_t sample_rate; ///< Sample rate the pulses are recorded with.
                        //    unsigned depth_bits;        ///< Sample depth in bits.
                        //    unsigned start_ago;         ///< Start of first pulse in number of samples ago.
                        //    unsigned end_ago;           ///< End of last pulse in number of samples ago.
  unsigned int num_pulses;
  int pulse[PD_MAX_PULSES]; ///< Width of pulses (high) in number of samples.
  int gap[PD_MAX_PULSES];   ///< Width of gaps between pulses (low) in number of samples.
#ifdef RSSI
  int rssi[PD_MAX_PULSES];
#endif
  //    int ook_low_estimate;       ///< Estimate for the OOK low level (base noise level) at beginning of package.
  //    int ook_high_estimate;      ///< Estimate for the OOK high level at end of package.
  //    int fsk_f1_est;             ///< Estimate for the F1 frequency for FSK.
  //    int fsk_f2_est;             ///< Estimate for the F2 frequency for FSK.
  float freq1_hz;
  //    float freq2_hz;
  //    float centerfreq_hz;
  //    float range_db;
  //    float rssi_db;
  //    float snr_db;
  //    float noise_db;
  int signalRssi;
  unsigned long signalDuration;
} pulse_data_t;

pulse_data_t *_pulseTrains;
volatile uint8_t _actualPulseTrain = 0;

void ICACHE_RAM_ATTR interruptHandler()
{
  if (!_enabledReceiver)
  {
    return;
  }
  volatile pulse_data_t &pulseTrain = _pulseTrains[_actualPulseTrain];
  volatile int *pulse = pulseTrain.pulse;
  volatile int *gap = pulseTrain.gap;
#ifdef RSSI
  volatile int *rssi = pulseTrain.rssi;
#endif

  const unsigned long now = micros();
  const unsigned int duration = now - _lastChange;

  if (duration > MINIMUM_PULSE_LENGTH && currentRssi > minimumRssi)
  {
#ifdef RSSI
    rssi[_nrpulses] = currentRssi;
#endif
    if (!digitalRead(RF_RECEIVER_GPIO))
    {
      pulse[_nrpulses] = duration;
      //      _nrpulses = (uint16_t)((_nrpulses + 1) % PD_MAX_PULSES);
    }
    else
    {
      gap[_nrpulses] = duration;
      _nrpulses = (uint16_t)((_nrpulses + 1) % PD_MAX_PULSES);
    }
    _lastChange = now;
  }
}

void setup()
{

  Serial.begin(921600);
  delay(1500);
  Serial.println();
  if (ELECHOUSE_cc1101.getCC1101())
  { // Check the CC1101 Spi connection.
    Serial.println("Connection OK");
  }
  else
  {
    Serial.println("Connection Error");
  }

  ELECHOUSE_cc1101.Init(); // must be set to initialize the cc1101!
  /*
    ELECHOUSE_cc1101.setCCMode(1);          // set config for internal transmission mode.
    ELECHOUSE_cc1101.setModulation(0);      // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    ELECHOUSE_cc1101.setMHZ(433.92);        // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    ELECHOUSE_cc1101.setDeviation(47.60);   // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
    ELECHOUSE_cc1101.setChannel(0);         // Set the Channelnumber from 0 to 255. Default is cahnnel 0.
    ELECHOUSE_cc1101.setChsp(199.95);       // The channel spacing is multiplied by the channel number CHAN and added to the base frequency in kHz. Value from 25.39 to 405.45. Default is 199.95 kHz.
    ELECHOUSE_cc1101.setRxBW(812.50);       // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
    ELECHOUSE_cc1101.setDRate(99.97);       // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
    ELECHOUSE_cc1101.setPA(10);             // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
    ELECHOUSE_cc1101.setSyncMode(2);        // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    ELECHOUSE_cc1101.setSyncWord(211, 145); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
    ELECHOUSE_cc1101.setAdrChk(0);          // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
    ELECHOUSE_cc1101.setAddr(0);            // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
    ELECHOUSE_cc1101.setWhiteData(0);       // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
    ELECHOUSE_cc1101.setPktFormat(0);       // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
    ELECHOUSE_cc1101.setLengthConfig(1);    // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
    ELECHOUSE_cc1101.setPacketLength(0);    // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
    ELECHOUSE_cc1101.setCrc(1);             // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
    ELECHOUSE_cc1101.setCRC_AF(0);          // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
    ELECHOUSE_cc1101.setDcFilterOff(0);     // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity).
    ELECHOUSE_cc1101.setManchester(0);      // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
    ELECHOUSE_cc1101.setFEC(0);             // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
    ELECHOUSE_cc1101.setPRE(0);             // Sets the minimum number of preamble bytes to be transmitted. Values: 0 : 2, 1 : 3, 2 : 4, 3 : 6, 4 : 8, 5 : 12, 6 : 16, 7 : 24
    ELECHOUSE_cc1101.setPQT(0);             // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
    ELECHOUSE_cc1101.setAppendStatus(0);    // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.
    */

  ELECHOUSE_cc1101.SetRx(433.92);
  Serial.println("Rx Mode");
  Serial.print("Minimum RSSI: ");
  Serial.println(MINRSSI);
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW);
  _pulseTrains = (pulse_data_t *)calloc(RECEIVER_BUFFER_SIZE, sizeof(pulse_data_t));
  int16_t interrupt = digitalPinToInterrupt(RF_RECEIVER_GPIO);
  attachInterrupt((uint8_t)interrupt, interruptHandler, CHANGE);
  _enabledReceiver = true;
}

byte buffer[256] = {0}; // was 61

void loop()
{

  //Checks whether something has been received.
  //When something is received we give some time to receive the message in full.(time in millis)
  currentRssi = ELECHOUSE_cc1101.getRssi();
  if (currentRssi > minimumRssi)
  {
    if (!receiveMode)
    {
      receiveMode = true;
      digitalWrite(ONBOARD_LED, HIGH);
      signalStart = micros();
      //     enableReceiver(receiverGpio);
      digitalWrite(ONBOARD_LED, HIGH);
      signalRssi = currentRssi;
      _lastChange = micros();
    }
  }
  else
  {
    if (receiveMode)
    {
      receiveMode = false;
      digitalWrite(ONBOARD_LED, LOW);
      _pulseTrains[_actualPulseTrain].num_pulses = _nrpulses;
      _pulseTrains[_actualPulseTrain].signalDuration = micros() - signalStart;
      _pulseTrains[_actualPulseTrain].signalRssi = signalRssi;
      int currentPulseTrain = _actualPulseTrain;
      _actualPulseTrain = (_actualPulseTrain + 1) % RECEIVER_BUFFER_SIZE;
      _nrpulses = 0;

// if (_pulseTrains[currentPulseTrain].num_pulses > PD_MIN_PULSES && _pulseTrains[currentPulseTrain].signalDuration > MINIMUM_SIGNAL_LENGTH)
      if ( _pulseTrains[currentPulseTrain].signalDuration > 1000)
      {
        Serial.println();
        Serial.print("Signal Train: ");
        Serial.println(currentPulseTrain);
        Serial.print("Signal length: ");
        Serial.println(_pulseTrains[currentPulseTrain].signalDuration);
        Serial.print("Signal RSSI: ");
        Serial.println(_pulseTrains[currentPulseTrain].signalRssi);
        Serial.print("Pulses: ");
        Serial.println(_pulseTrains[currentPulseTrain].num_pulses);
        Serial.print("Time: ");
        Serial.println(micros() / 1000);

        
      for (int i = 0; i < _pulseTrains[currentPulseTrain].num_pulses; i++)
      {
        Serial.print("+");
        Serial.print(_pulseTrains[currentPulseTrain].pulse[i]);
        Serial.print("-");
        Serial.print(_pulseTrains[currentPulseTrain].gap[i]);
      }
      Serial.println();
      
      }
    }
  }
}