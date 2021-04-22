/*
  rtl_433_ESP - pilight 433.92 MHz protocols library for Arduino
  Copyright (c) 2016 Puuu.  All right reserved.

  Project home: https://github.com/puuu/rtl_433_ESP/
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with library. If not, see <http://www.gnu.org/licenses/>
*/

#include <rtl_433_ESP.h>
#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include "tools/aprintf.h"
#include "log.h"

// ESP32 doesn't define ICACHE_RAM_ATTR
#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR IRAM_ATTR
#endif

extern "C"
{
#include "bitbuffer.h"
#include "pulse_detect.h"
#include "pulse_demod.h"
#include "list.h"
// #include "rtl_devices.h"
#include "r_api.h"
#include "r_private.h"
#include "rtl_433.h"
#include "rtl_433_devices.h"
#include "fatal.h"
  // #include "decoder.h"
}

/**
 * Is the receiver currently receving a signal
 */
static bool receiveMode = false;

static bool signalComplete = false;

/**
 * Timestamp in micros for start of current signal
 */
static unsigned long signalStart = micros();

/**
 * Timestamp in micros for end of most recent signal aka start of current gap
 */
static unsigned long gapStart = micros();

static unsigned long loopLength = micros();

static unsigned long signalEnd = micros();

pulse_data_t *_pulseTrains;

int rtl_433_ESP::messageCount = 0;
int rtl_433_ESP::currentRssi = 0;
int rtl_433_ESP::signalRssi = 0;
bool rtl_433_ESP::_enabledReceiver = false;
volatile uint8_t rtl_433_ESP::_actualPulseTrain = 0;
uint8_t rtl_433_ESP::_avaiablePulseTrain = 0;
volatile unsigned long rtl_433_ESP::_lastChange = 0; // Timestamp of previous edge
int rtl_433_ESP::rtlVerbose = 0;
volatile uint16_t rtl_433_ESP::_nrpulses = 0;

int16_t rtl_433_ESP::_interrupt = NOT_AN_INTERRUPT;
static byte receiverGpio = -1;

// static byte modulation = CC1101_ASK; // ASK ( Default )

static byte modulation = CC1101_2FSK;

r_cfg_t rtl_433_ESP::g_cfg;

void rtl_433_ESP::rtlSetup(r_cfg_t *cfg)
{
  unsigned i;
#ifdef MEMORY_DEBUG
  logprintfLn(LOG_INFO, "sizeof(*cfg->demod) %d", sizeof(*cfg->demod));
#endif
  r_init_cfg(cfg);

  cfg->conversion_mode = CONVERT_SI; // Default all output to Celcius
  cfg->num_r_devices = NUMOFDEVICES;
  cfg->devices = (r_device *)calloc(cfg->num_r_devices, sizeof(r_device));
  if (!cfg->devices)
    FATAL_CALLOC("cfg->devices");

#ifndef MY_DEVICES
  // This is a generated fragment from tools/update_rtl_433_devices.sh

  memcpy(&cfg->devices[0], &acurite_rain_896, sizeof(r_device));
  memcpy(&cfg->devices[1], &acurite_th, sizeof(r_device));
  memcpy(&cfg->devices[2], &acurite_txr, sizeof(r_device));
  memcpy(&cfg->devices[3], &acurite_986, sizeof(r_device));
  memcpy(&cfg->devices[4], &acurite_606, sizeof(r_device));
  memcpy(&cfg->devices[5], &acurite_00275rm, sizeof(r_device));
  memcpy(&cfg->devices[6], &acurite_590tx, sizeof(r_device));
  memcpy(&cfg->devices[7], &akhan_100F14, sizeof(r_device));
  memcpy(&cfg->devices[8], &alectov1, sizeof(r_device));
  memcpy(&cfg->devices[9], &ambientweather_tx8300, sizeof(r_device));
  memcpy(&cfg->devices[10], &ambientweather_wh31e, sizeof(r_device));
  memcpy(&cfg->devices[11], &archos_tbh, sizeof(r_device));
  memcpy(&cfg->devices[12], &auriol_afw2a1, sizeof(r_device));
  memcpy(&cfg->devices[13], &auriol_ahfl, sizeof(r_device));
  memcpy(&cfg->devices[14], &auriol_hg02832, sizeof(r_device));
  memcpy(&cfg->devices[15], &blueline, sizeof(r_device));
  memcpy(&cfg->devices[16], &blyss, sizeof(r_device));
  memcpy(&cfg->devices[17], &brennenstuhl_rcs_2044, sizeof(r_device));
  memcpy(&cfg->devices[18], &bresser_3ch, sizeof(r_device));
  memcpy(&cfg->devices[19], &bresser_5in1, sizeof(r_device));
  memcpy(&cfg->devices[20], &bresser_6in1, sizeof(r_device));
  memcpy(&cfg->devices[21], &bresser_7in1, sizeof(r_device));
  memcpy(&cfg->devices[22], &bt_rain, sizeof(r_device));
  memcpy(&cfg->devices[23], &burnhardbbq, sizeof(r_device));
  memcpy(&cfg->devices[24], &calibeur_RF104, sizeof(r_device));
  memcpy(&cfg->devices[25], &cardin, sizeof(r_device));
  memcpy(&cfg->devices[26], &chuango, sizeof(r_device));
  memcpy(&cfg->devices[27], &companion_wtr001, sizeof(r_device));
  memcpy(&cfg->devices[28], &danfoss_CFR, sizeof(r_device));
  memcpy(&cfg->devices[29], &digitech_xc0324, sizeof(r_device));
  memcpy(&cfg->devices[30], &directv, sizeof(r_device));
  memcpy(&cfg->devices[31], &dish_remote_6_3, sizeof(r_device));
  memcpy(&cfg->devices[32], &ecodhome, sizeof(r_device));
  memcpy(&cfg->devices[33], &ecowitt, sizeof(r_device));
  memcpy(&cfg->devices[34], &eurochron_efth800, sizeof(r_device));
  memcpy(&cfg->devices[35], &elro_db286a, sizeof(r_device));
  memcpy(&cfg->devices[36], &elv_em1000, sizeof(r_device));
  memcpy(&cfg->devices[37], &elv_ws2000, sizeof(r_device));
  memcpy(&cfg->devices[38], &emontx, sizeof(r_device));
  memcpy(&cfg->devices[39], &esic_emt7110, sizeof(r_device));
  memcpy(&cfg->devices[40], &esperanza_ews, sizeof(r_device));
  memcpy(&cfg->devices[41], &eurochron, sizeof(r_device));
  memcpy(&cfg->devices[42], &fineoffset_WH2, sizeof(r_device));
  memcpy(&cfg->devices[43], &fineoffset_WH25, sizeof(r_device));
  memcpy(&cfg->devices[44], &fineoffset_WH51, sizeof(r_device));
  memcpy(&cfg->devices[45], &fineoffset_WH0530, sizeof(r_device));
  memcpy(&cfg->devices[46], &fineoffset_wh1050, sizeof(r_device));
  memcpy(&cfg->devices[47], &fineoffset_wh1080, sizeof(r_device));
  memcpy(&cfg->devices[48], &fineoffset_wh1080_fsk, sizeof(r_device));
  memcpy(&cfg->devices[49], &fs20, sizeof(r_device));
  memcpy(&cfg->devices[50], &ft004b, sizeof(r_device));
  memcpy(&cfg->devices[51], &generic_motion, sizeof(r_device));
  memcpy(&cfg->devices[52], &generic_remote, sizeof(r_device));
  memcpy(&cfg->devices[53], &generic_temperature_sensor, sizeof(r_device));
  memcpy(&cfg->devices[54], &gt_tmbbq05, sizeof(r_device));
  memcpy(&cfg->devices[55], &gt_wt_02, sizeof(r_device));
  memcpy(&cfg->devices[56], &gt_wt_03, sizeof(r_device));
  memcpy(&cfg->devices[57], &hcs200, sizeof(r_device));
  memcpy(&cfg->devices[58], &holman_ws5029pcm, sizeof(r_device));
  memcpy(&cfg->devices[59], &honeywell_wdb, sizeof(r_device));
  memcpy(&cfg->devices[60], &ht680, sizeof(r_device));
  memcpy(&cfg->devices[61], &ikea_sparsnas, sizeof(r_device));
  memcpy(&cfg->devices[62], &infactory, sizeof(r_device));
  memcpy(&cfg->devices[63], &kw9015b, sizeof(r_device));
  memcpy(&cfg->devices[64], &interlogix, sizeof(r_device));
  memcpy(&cfg->devices[65], &intertechno, sizeof(r_device));
  memcpy(&cfg->devices[66], &kedsum, sizeof(r_device));
  memcpy(&cfg->devices[67], &kerui, sizeof(r_device));
  memcpy(&cfg->devices[68], &lacrossetx, sizeof(r_device));
  memcpy(&cfg->devices[69], &lacrosse_breezepro, sizeof(r_device));
  memcpy(&cfg->devices[70], &lacrosse_r1, sizeof(r_device));
  memcpy(&cfg->devices[71], &lacrosse_th3, sizeof(r_device));
  memcpy(&cfg->devices[72], &lacrosse_tx141x, sizeof(r_device));
  memcpy(&cfg->devices[73], &lacrosse_tx29, sizeof(r_device));
  memcpy(&cfg->devices[74], &lacrosse_tx35, sizeof(r_device));
  memcpy(&cfg->devices[75], &lacrosse_wr1, sizeof(r_device));
  memcpy(&cfg->devices[76], &lacrosse_ws7000, sizeof(r_device));
  memcpy(&cfg->devices[77], &lacrossews, sizeof(r_device));
  memcpy(&cfg->devices[78], &lightwave_rf, sizeof(r_device));
  memcpy(&cfg->devices[79], &maverick_et73, sizeof(r_device));
  memcpy(&cfg->devices[80], &mebus433, sizeof(r_device));
  memcpy(&cfg->devices[81], &missil_ml0757, sizeof(r_device));
  memcpy(&cfg->devices[82], &new_template, sizeof(r_device));
  memcpy(&cfg->devices[83], &nexus, sizeof(r_device));
  memcpy(&cfg->devices[84], &nice_flor_s, sizeof(r_device));
  memcpy(&cfg->devices[85], &opus_xt300, sizeof(r_device));
  memcpy(&cfg->devices[86], &oregon_scientific_sl109h, sizeof(r_device));
  memcpy(&cfg->devices[87], &oregon_scientific_v1, sizeof(r_device));
  memcpy(&cfg->devices[88], &philips_aj3650, sizeof(r_device));
  memcpy(&cfg->devices[89], &philips_aj7010, sizeof(r_device));
  memcpy(&cfg->devices[90], &prologue, sizeof(r_device));
  memcpy(&cfg->devices[91], &quhwa, sizeof(r_device));
  memcpy(&cfg->devices[92], &rftech, sizeof(r_device));
  memcpy(&cfg->devices[93], &rubicson, sizeof(r_device));
  memcpy(&cfg->devices[94], &rubicson_48659, sizeof(r_device));
  memcpy(&cfg->devices[95], &s3318p, sizeof(r_device));
  memcpy(&cfg->devices[96], &silvercrest, sizeof(r_device));
  memcpy(&cfg->devices[97], &skylink_motion, sizeof(r_device));
  memcpy(&cfg->devices[98], &smoke_gs558, sizeof(r_device));
  memcpy(&cfg->devices[99], &solight_te44, sizeof(r_device));
  memcpy(&cfg->devices[100], &springfield, sizeof(r_device));
  memcpy(&cfg->devices[101], &tfa_30_3221, sizeof(r_device));
  memcpy(&cfg->devices[102], &tfa_drop_303233, sizeof(r_device));
  memcpy(&cfg->devices[103], &tfa_marbella, sizeof(r_device));
  memcpy(&cfg->devices[104], &tfa_pool_thermometer, sizeof(r_device));
  memcpy(&cfg->devices[105], &tfa_twin_plus_303049, sizeof(r_device));
  memcpy(&cfg->devices[106], &thermopro_tp11, sizeof(r_device));
  memcpy(&cfg->devices[107], &thermopro_tp12, sizeof(r_device));
  memcpy(&cfg->devices[108], &thermopro_tx2, sizeof(r_device));
  memcpy(&cfg->devices[109], &ts_ft002, sizeof(r_device));
  memcpy(&cfg->devices[110], &visonic_powercode, sizeof(r_device));
  memcpy(&cfg->devices[111], &waveman, sizeof(r_device));
  memcpy(&cfg->devices[112], &wg_pb12v1, sizeof(r_device));
  memcpy(&cfg->devices[113], &ws2032, sizeof(r_device));
  memcpy(&cfg->devices[114], &wssensor, sizeof(r_device));
  memcpy(&cfg->devices[115], &wt1024, sizeof(r_device));
  memcpy(&cfg->devices[116], &X10_RF, sizeof(r_device));
  memcpy(&cfg->devices[117], &x10_sec, sizeof(r_device));

  // end of fragement

#else
  memcpy(&cfg->devices[0], &skylink_motion, sizeof(r_device));
  memcpy(&cfg->devices[1], &prologue, sizeof(r_device));
  memcpy(&cfg->devices[2], &acurite_986, sizeof(r_device));
  memcpy(&cfg->devices[3], &philips_aj3650, sizeof(r_device));
  memcpy(&cfg->devices[4], &fineoffset_WH51, sizeof(r_device));
#endif

// r_device [177]{(struct r_device)silvercrest, (struct r_device)rubicson, (struct r_device)prologue, (struct r_device)waveman, (struct r_device)new_template, (struct r_device)elv_em1000, (struct r_device)elv_ws2000, (struct r_device)lacrossetx, (struct r_device)new_template, (struct r_device)acurite_rain_896, (struct r_device)acurite_th, (struct r_device)oregon_scientific, (struct r_device)mebus433, (struct r_device)intertechno, (struct r_device)newkaku, (struct r_device)alectov1, (struct r_device)cardin, (struct r_device)fineoffset_WH2, (struct r_device)nexus, (struct r_device)ambient_weather, (struct r_device)calibeur_RF104, (struct r_device)X10_RF, (struct r_device)dsc_security, (struct r_device)brennenstuhl_rcs_2044, (struct r_device)gt_wt_02, (struct r_device)danfoss_CFR, (struct r_device)new_template, (struct r_device)new_template, (struct r_device)chuango, (struct r_device)generic_remote, (struct r_device)tfa_twin_plus_303049, (struct r_device)fineoffset_wh1080, (struct r_device)wt450, (struct r_device)lacrossews, (struct r_device)esperanza_ews, (struct r_device)efergy_e2_classic, (struct r_device)kw9015b, (struct r_device)generic_temperature_sensor, (struct r_device)wg_pb12v1, (struct r_device)acurite_txr, (struct r_device)acurite_986, (struct r_device)hideki_ts04, (struct r_device)oil_watchman, (struct r_device)current_cost, (struct r_device)emontx, (struct r_device)ht680, (struct r_device)s3318p, (struct r_device)akhan_100F14, (struct r_device)quhwa, (struct r_device)oregon_scientific_v1, (struct r_device)proove, (struct r_device)bresser_3ch, (struct r_device)springfield, (struct r_device)oregon_scientific_sl109h, (struct r_device)acurite_606, (struct r_device)tfa_pool_thermometer, (struct r_device)kedsum, (struct r_device)blyss, (struct r_device)steelmate, (struct r_device)schraeder, (struct r_device)lightwave_rf, (struct r_device)elro_db286a, (struct r_device)efergy_optical, (struct r_device)hondaremote, (struct r_device)new_template, (struct r_device)new_template, (struct r_device)radiohead_ask, (struct r_device)kerui, (struct r_device)fineoffset_wh1050, (struct r_device)honeywell, (struct r_device)maverick_et73x, (struct r_device)rftech, (struct r_device)lacrosse_tx141x, (struct r_device)acurite_00275rm, (struct r_device)lacrosse_tx35, (struct r_device)lacrosse_tx29, (struct r_device)vaillant_vrt340f, (struct r_device)fineoffset_WH25, (struct r_device)fineoffset_WH0530, (struct r_device)ibis_beacon, (struct r_device)oil_standard, (struct r_device)tpms_citroen, (struct r_device)oil_standard_ask, (struct r_device)thermopro_tp11, (struct r_device)solight_te44, (struct r_device)smoke_gs558, (struct r_device)generic_motion, (struct r_device)tpms_toyota, (struct r_device)tpms_ford, (struct r_device)tpms_renault, (struct r_device)infactory, (struct r_device)ft004b, (struct r_device)fordremote, (struct r_device)philips_aj3650, (struct r_device)schrader_EG53MA4, (struct r_device)nexa, (struct r_device)thermopro_tp12, (struct r_device)ge_coloreffects, (struct r_device)x10_sec, (struct r_device)interlogix, (struct r_device)dish_remote_6_3, (struct r_device)ss_sensor, (struct r_device)sensible_living, (struct r_device)m_bus_mode_c_t, (struct r_device)m_bus_mode_s, (struct r_device)m_bus_mode_r, (struct r_device)m_bus_mode_f, (struct r_device)wssensor, (struct r_device)wt1024, (struct r_device)tpms_pmv107j, (struct r_device)ttx201, (struct r_device)ambientweather_tx8300, (struct r_device)ambientweather_wh31e, (struct r_device)maverick_et73, (struct r_device)honeywell_wdb, (struct r_device)honeywell_wdb_fsk, (struct r_device)esa_energy, (struct r_device)bt_rain, (struct r_device)bresser_5in1, (struct r_device)digitech_xc0324, (struct r_device)opus_xt300, (struct r_device)fs20, (struct r_device)tpms_jansite, (struct r_device)lacrosse_ws7000, (struct r_device)ts_ft002, (struct r_device)companion_wtr001, (struct r_device)ecowitt, (struct r_device)directv, (struct r_device)eurochron, (struct r_device)ikea_sparsnas, (struct r_device)hcs200, (struct r_device)tfa_303196, (struct r_device)rubicson_48659, (struct r_device)holman_ws5029pcm, (struct r_device)philips_aj7010, (struct r_device)esic_emt7110, (struct r_device)gt_tmbbq05, (struct r_device)gt_wt_03, (struct r_device)norgo, (struct r_device)tpms_elantra2012, (struct r_device)auriol_hg02832, (struct r_device)fineoffset_WH51, (struct r_device)holman_ws5029pwm, (struct r_device)archos_tbh, (struct r_device)ws2032, (struct r_device)auriol_afw2a1, (struct r_device)tfa_drop_303233, (struct r_device)dsc_security_ws4945, (struct r_device)ert_scm, (struct r_device)klimalogg, (struct r_device)visonic_powercode, (struct r_device)eurochron_efth800, (struct r_device)cotech_36_7959, (struct r_device)scmplus, (struct r_device)fineoffset_wh1080_fsk, (struct r_device)tpms_abarth124, (struct r_device)missil_ml0757, (struct r_device)sharp_spc775, (struct r_device)insteon, (struct r_device)ert_idm, (struct r_device)ert_netidm, (struct r_device)thermopro_tx2, (struct r_device)acurite_590tx, (struct r_device)secplus_v2, (struct r_device)tfa_30_3221, (struct r_device)lacrosse_breezepro, (struct r_device)somfy_rts, (struct r_device)schrader_SMD3MA4, (struct r_device)nice_flor_s, (struct r_device)lacrosse_wr1, (struct r_device)lacrosse_th3, (struct r_device)bresser_6in1, (struct r_device)bresser_7in1, (struct r_device)ecodhome, (struct r_device)lacrosse_r1, (struct r_device)blueline, (struct r_device)burnhardbbq}

// logprintfLn(LOG_INFO, "Location of r_devices: %p", (void *)&r_devices);
// logprintfLn(LOG_INFO, "Location of cfg: %p", (void *)&cfg);
// logprintfLn(LOG_INFO, "cfg size %d", sizeof(r_cfg_t));
// logprintfLn(LOG_INFO, "Location of cfg->devices: %p", (void *)&cfg->devices);
#ifdef MEMORY_DEBUG
  logprintfLn(LOG_INFO, "size of bitbuffer: %d", sizeof(bitbuffer_t));
  logprintfLn(LOG_INFO, "size of pulse_data: %d", sizeof(pulse_data_t));
#endif

  for (i = 0; i < cfg->num_r_devices; i++)
  {
    cfg->devices[i].protocol_num = i + 1;
    // These pulse demods have been tested (85), ymmv for the others
    /*
    if (cfg->devices[i].modulation == OOK_PULSE_PPM || cfg->devices[i].modulation == OOK_PULSE_PWM)
    {
      numberEnabled++;
    }
    else
    {
      cfg->devices[i].disabled = 1;
    }
    */
  }
#ifdef DEMOD_DEBUG
  logprintfLn(LOG_INFO, "# of device(s) configured %d", cfg->num_r_devices);
  logprintfLn(LOG_INFO, "ssizeof(r_device): %d", sizeof(r_device));
  logprintfLn(LOG_INFO, "cfg->devices size: %d", sizeof(r_device) * cfg->num_r_devices);
#endif

#ifdef RTL_DEBUG
  cfg->verbosity = RTL_DEBUG; //0=normal, 1=verbose, 2=verbose decoders, 3=debug decoders, 4=trace decoding.
#else
  cfg->verbosity = rtlVerbose; //0=normal, 1=verbose, 2=verbose decoders, 3=debug decoders, 4=trace decoding.
#endif
  register_all_protocols(cfg, 0);
}

void rtl_433_ESP::initReceiver(byte inputPin, float receiveFrequency)
{
  receiverGpio = inputPin;
#ifdef MEMORY_DEBUG
  logprintfLn(LOG_INFO, "Pre initReceiver: %d", ESP.getFreeHeap());
#endif
#ifdef DEMOD_DEBUG
  logprintfLn(LOG_INFO, "CC1101 gpio receive pin: %d", inputPin);
  logprintfLn(LOG_INFO, "CC1101 receive frequency: %f", receiveFrequency);
#endif
  r_cfg_t *cfg = &g_cfg;

  rtlSetup(cfg);

  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(modulation);
  ELECHOUSE_cc1101.setSyncMode(0);                     // Disable sync mode
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG0, 0x0E);   // Enable carrier sense for GDO0
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL1, 0x30); // Carrier sense relative +6db
  // Needs further tuning for FSK
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_DEVIATN, 0x41); // 66
  // ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG3, 0x93);   // This is the ELECHOUSE Default
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG4, 0x1f); // Starts as a 7, C9 works sorts

  ELECHOUSE_cc1101.SetRx(receiveFrequency);

  resetReceiver();
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW);

  logprintfLn(LOG_INFO, "CC1101_MDMCFG1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_MDMCFG1));
  logprintfLn(LOG_INFO, "CC1101_MDMCFG2: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_MDMCFG2));
  logprintfLn(LOG_INFO, "CC1101_MDMCFG3: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_MDMCFG3));
  logprintfLn(LOG_INFO, "CC1101_MDMCFG4: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_MDMCFG4));
  logprintfLn(LOG_INFO, "CC1101_DEVIATN: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_DEVIATN));
  logprintfLn(LOG_INFO, "CC1101_AGCCTRL0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_AGCCTRL0));
  logprintfLn(LOG_INFO, "CC1101_AGCCTRL1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_AGCCTRL1));
  logprintfLn(LOG_INFO, "CC1101_AGCCTRL2: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_AGCCTRL2));
  logprintfLn(LOG_INFO, "CC1101_IOCFG0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_IOCFG0));
  logprintfLn(LOG_INFO, "CC1101_IOCFG1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_IOCFG1));
  logprintfLn(LOG_INFO, "CC1101_IOCFG2: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_IOCFG2));
  logprintfLn(LOG_INFO, "CC1101_FIFOTHR: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FIFOTHR));
  logprintfLn(LOG_INFO, "CC1101_SYNC0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_SYNC0));
  logprintfLn(LOG_INFO, "CC1101_SYNC1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_SYNC1));

  logprintfLn(LOG_INFO, "CC1101_PKTLEN: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_PKTLEN));
  logprintfLn(LOG_INFO, "CC1101_PKTCTRL0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_PKTCTRL0));
  logprintfLn(LOG_INFO, "CC1101_PKTCTRL1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_PKTCTRL1));
  logprintfLn(LOG_INFO, "CC1101_ADDR: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_ADDR));
  logprintfLn(LOG_INFO, "CC1101_CHANNR: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_CHANNR));
  logprintfLn(LOG_INFO, "CC1101_FSCTRL0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FSCTRL0));
  logprintfLn(LOG_INFO, "CC1101_FSCTRL1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FSCTRL1));
  logprintfLn(LOG_INFO, "CC1101_FREQ0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FREQ0));
  logprintfLn(LOG_INFO, "CC1101_FREQ1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FREQ1));
  logprintfLn(LOG_INFO, "CC1101_FREQ2: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FREQ2));
  logprintfLn(LOG_INFO, "CC1101_MCSM0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_MCSM0));
  logprintfLn(LOG_INFO, "CC1101_MCSM1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_MCSM1));
  logprintfLn(LOG_INFO, "CC1101_MCSM2: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_MCSM2));
  logprintfLn(LOG_INFO, "CC1101_FOCCFG: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FOCCFG));

  logprintfLn(LOG_INFO, "CC1101_BSCFG: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_BSCFG));
  logprintfLn(LOG_INFO, "CC1101_WOREVT0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_WOREVT0));
  logprintfLn(LOG_INFO, "CC1101_WOREVT1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_WOREVT1));
  logprintfLn(LOG_INFO, "CC1101_WORCTRL: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_WORCTRL));
  logprintfLn(LOG_INFO, "CC1101_FREND0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FREND0));
  logprintfLn(LOG_INFO, "CC1101_FREND1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FREND1));
  logprintfLn(LOG_INFO, "CC1101_FSCAL0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FSCAL0));
  logprintfLn(LOG_INFO, "CC1101_FSCAL1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FSCAL1));
  logprintfLn(LOG_INFO, "CC1101_FSCAL2: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FSCAL2));
  logprintfLn(LOG_INFO, "CC1101_FSCAL3: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_FSCAL3));
  logprintfLn(LOG_INFO, "CC1101_RCCTRL0: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_RCCTRL0));
  logprintfLn(LOG_INFO, "CC1101_RCCTRL1: 0x%.2x", ELECHOUSE_cc1101.SpiReadReg(CC1101_RCCTRL1));

#ifdef MEMORY_DEBUG
  logprintfLn(LOG_INFO, "Post initReceiver: %d", ESP.getFreeHeap());
#endif
}

int rtl_433_ESP::receivePulseTrain()
{

  if (_pulseTrains[_avaiablePulseTrain].num_pulses > 0)
  {
    uint8_t _currentTrain = _avaiablePulseTrain;
    _avaiablePulseTrain = (_avaiablePulseTrain + 1) % RECEIVER_BUFFER_SIZE;
    return _currentTrain;
  }
  return -1;
}

void ICACHE_RAM_ATTR rtl_433_ESP::interruptSignal()
{
  if (!_enabledReceiver || !receiveMode)
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

  if (duration > MINIMUM_PULSE_LENGTH)
  {
#ifdef RSSI
    rssi[_nrpulses] = currentRssi;
#endif
    if (!digitalRead(receiverGpio))
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

void ICACHE_RAM_ATTR rtl_433_ESP::interruptCarrierSense()
{
  if (!_enabledReceiver)
  {
    return;
  }
  if (digitalRead(CC1101_GDO0))
  {
    if (!receiveMode)
    {
      receiveMode = true;
      signalStart = micros();
      //  enableReceiver(receiverGpio);
      digitalWrite(ONBOARD_LED, HIGH);
      _lastChange = micros();
      _nrpulses = 0;
    }
  }
  else
  {
    gapStart = signalEnd; // previous gap start
    signalEnd = micros();
    receiveMode = false;
    signalRssi = currentRssi;
    digitalWrite(ONBOARD_LED, LOW);
    if (_nrpulses > PD_MIN_PULSES && micros() - signalStart > PD_MIN_SIGNAL_LENGTH)
    {
      _pulseTrains[_actualPulseTrain].num_pulses = _nrpulses;
      _pulseTrains[_actualPulseTrain].signalDuration = micros() - signalStart;
      _pulseTrains[_actualPulseTrain].signalRssi = currentRssi;
      _pulseTrains[_actualPulseTrain].signalTime = millis();
      _pulseTrains[_actualPulseTrain].signalNumber = messageCount;
      _actualPulseTrain = (_actualPulseTrain + 1) % RECEIVER_BUFFER_SIZE;
      messageCount++;
      signalComplete = true;
    }
  }
}

void rtl_433_ESP::resetReceiver()
{
  for (unsigned int i = 0; i < RECEIVER_BUFFER_SIZE; i++)
  {
    _pulseTrains[i].num_pulses = 0;
  }
  _avaiablePulseTrain = 0;
  _actualPulseTrain = 0;
  _nrpulses = 0;

  receiveMode = false;
  signalStart = micros();
}

void rtl_433_ESP::enableReceiver(byte inputPin)
{
  int16_t interrupt = digitalPinToInterrupt(inputPin);
  if (_interrupt == interrupt)
  {
    return;
  }
  if (_interrupt >= 0)
  {
    detachInterrupt((uint8_t)_interrupt);
  }
  _interrupt = interrupt;

  if (interrupt >= 0)
  {
    attachInterrupt((uint8_t)interrupt, interruptSignal, CHANGE);
    attachInterrupt((uint8_t)digitalPinToInterrupt(CC1101_GDO0), interruptCarrierSense, CHANGE);
    _enabledReceiver = true;
  }
}

void rtl_433_ESP::disableReceiver() { _enabledReceiver = false; }

void rtl_433_ESP::loop()
{
  loopLength = micros();
  if (_enabledReceiver)
  {
    currentRssi = ELECHOUSE_cc1101.getRssi();

    /*
    if (signalComplete)
    {
#ifdef DEMOD_DEBUG
      alogprintfLn(LOG_INFO, " ");
      logprintf(LOG_INFO, "Time: %lu", millis() / 1000);
      alogprintf(LOG_INFO, ", Signal length: %lu", signalEnd - signalStart);
      alogprintf(LOG_INFO, ", Gap length: %lu", signalStart - gapStart);
      alogprintf(LOG_INFO, ", Signal RSSI: %d", signalRssi);
      alogprintf(LOG_INFO, ", train: %d", _actualPulseTrain);
      alogprintf(LOG_INFO, ", messageCount: %d", messageCount);
      alogprintfLn(LOG_INFO, ", pulses: %d", _nrpulses);
#endif
      signalComplete = false;
    }
    */

    int _receiveTrain = receivePulseTrain();

    if (_receiveTrain != -1)
    {
      pulse_data_t *rtl_pulses = (pulse_data_t *)calloc(1, sizeof(pulse_data_t));
      memcpy(rtl_pulses, (char *)&_pulseTrains[_receiveTrain], sizeof(pulse_data_t));
      _pulseTrains[_receiveTrain].num_pulses = 0; // Make pulse train available for next train
#ifdef DEMOD_DEBUG
      alogprintfLn(LOG_INFO, " ");
      logprintf(LOG_INFO, "Time: %lu", rtl_pulses->signalTime);
      alogprintf(LOG_INFO, ", Signal length: %lu", rtl_pulses->signalDuration);
      alogprintf(LOG_INFO, ", Signal RSSI: %d", rtl_pulses->signalRssi);
      alogprintf(LOG_INFO, ", train: %d", _actualPulseTrain);
      alogprintf(LOG_INFO, ", messageCount: %d", rtl_pulses->signalNumber);
      alogprintfLn(LOG_INFO, ", pulses: %d", rtl_pulses->num_pulses);
#endif
      if (rtl_pulses->num_pulses > PD_MIN_PULSES && rtl_pulses->signalDuration > PD_MIN_SIGNAL_LENGTH)
      {
        unsigned long signalProcessingStart = micros();
        rtl_pulses->sample_rate = 1.0e6;
#ifdef RAW_SIGNAL_DEBUG
        logprintf(LOG_INFO, "RAW (%d): ", rtl_pulses->signalDuration);
        for (int i = 0; i < rtl_pulses->num_pulses; i++)
        {
          alogprintf(LOG_INFO, "+%d", rtl_pulses->pulse[i]);
          alogprintf(LOG_INFO, "-%d", rtl_pulses->gap[i]);
#ifdef RSSI
          alogprintf(LOG_INFO, "(%d)", rtl_pulses->rssi[i]);
#endif
        }
        alogprintfLn(LOG_INFO, " ");
#endif
#ifdef MEMORY_DEBUG
        logprintfLn(LOG_INFO, "Pre run_ook_demods: %d", ESP.getFreeHeap());
#endif
        r_cfg_t *cfg = &g_cfg;
        cfg->demod->pulse_data = rtl_pulses;
        int events = 0;
        switch (modulation)
        {
        case CC1101_2FSK:
        case CC1101_4FSK:
          events = run_fsk_demods(&cfg->demod->r_devs, rtl_pulses);
          break;
        case CC1101_ASK:
          events = run_ook_demods(&cfg->demod->r_devs, rtl_pulses);
          break;
        default:
          logprintfLn(LOG_ERR, "Unsupported modulation %u", modulation);
        }
#ifdef DEMOD_DEBUG
        logprintfLn(LOG_INFO, "# of messages decoded %d", events);
#endif
        if (events == 0)
        {
          alogprintfLn(LOG_INFO, " ");
          logprintf(LOG_INFO, "Unparsed Signal length: %lu", rtl_pulses->signalDuration);
          alogprintf(LOG_INFO, ", Signal RSSI: %d", rtl_pulses->signalRssi);
          alogprintf(LOG_INFO, ", train: %d", _actualPulseTrain);
          alogprintf(LOG_INFO, ", messageCount: %d", messageCount);
          alogprintfLn(LOG_INFO, ", pulses: %d", rtl_pulses->num_pulses);

          logprintf(LOG_INFO, "RAW (%d): ", rtl_pulses->signalDuration);
          for (int i = 0; i < rtl_pulses->num_pulses; i++)
          {
#ifdef RSSI
            alogprintf(LOG_INFO, "(%d)", rtl_pulses->rssi[i]);
#endif
            alogprintf(LOG_INFO, "+%d", rtl_pulses->pulse[i]);
            alogprintf(LOG_INFO, "-%d", rtl_pulses->gap[i]);
          }
          alogprintfLn(LOG_INFO, " ");
          alogprintfLn(LOG_INFO, " ");
          // Send a note saying unparsed signal signal received
#ifdef PUBLISH_UNPARSED
          data_t *data;
          /* clang-format off */
  data = data_make(
                "model", "",      DATA_STRING,  "unknown",
                "protocol", "",   DATA_STRING,  "signal parsing failed",
                "duration", "",   DATA_INT,     rtl_pulses->signalDuration,
                "signalRssi", "", DATA_INT,     rtl_pulses->signalRssi,
                "pulses", "",     DATA_INT,     rtl_pulses->num_pulses,
                "train", "",      DATA_INT,     _actualPulseTrain,
                "messageCount", "", DATA_INT,   messageCount,
                "_enabledReceiver", "", DATA_INT, _enabledReceiver,
                "receiveMode", "", DATA_INT,    receiveMode,
                "currentRssi", "", DATA_INT,    currentRssi,
                "modulation", "",  DATA_INT,     modulation,
                NULL);
          /* clang-format on */

          r_cfg_t *cfg = &g_cfg;
          data_print_jsons(data, cfg->messageBuffer, cfg->bufferSize);
          (cfg->callback)(cfg->messageBuffer);
          data_free(data);
#endif
        }

#ifdef MEMORY_DEBUG
        logprintfLn(LOG_INFO, "Signal processing time: %lu", micros() - signalProcessingStart);
        logprintfLn(LOG_INFO, "Post run_ook_demods memory %d", ESP.getFreeHeap());
#endif
      }
      free(rtl_pulses);
    }
    // logprintfLn(LOG_INFO, "Signal processing time: %lu", micros() - loopLength);
  }
}

rtl_433_ESP::rtl_433_ESP(int8_t outputPin)
{
  _outputPin = outputPin;

  if (_outputPin >= 0)
  {
    pinMode((uint8_t)_outputPin, OUTPUT);
    digitalWrite((uint8_t)_outputPin, LOW);
  }

  _pulseTrains = (pulse_data_t *)calloc(RECEIVER_BUFFER_SIZE, sizeof(pulse_data_t));
}

void rtl_433_ESP::setCallback(rtl_433_ESPCallBack callback, char *messageBuffer, int bufferSize)
{
  r_cfg_t *cfg = &g_cfg;
  cfg->callback = callback;
  cfg->messageBuffer = messageBuffer;
  cfg->bufferSize = bufferSize;
  // logprintfLn(LOG_INFO, "setCallback location: %p", cfg->callback);
}

void rtl_433_ESP::setDebug(int debug)
{
  r_cfg_t *cfg = &g_cfg;
  rtlVerbose = debug;
  // cfg->devices[debug].verbose = 3;
  // cfg->demod->r_devs.elems[debug].verbose = 3;
  logprintfLn(LOG_INFO, "Setting rtl_433 debug to: %d %s", cfg->devices[debug].verbose, cfg->devices[debug].name);
}

void rtl_433_ESP::setModulation(byte _modulation)
{
  if (_modulation >= 0 && _modulation <= 4)
  {
    modulation = _modulation;
    logprintfLn(LOG_INFO, "Setting modulation to: %d", modulation);
    ELECHOUSE_cc1101.setModulation(modulation);
  }
  else
  {
    logprintfLn(LOG_ERR, "Invalid modulation setting", _modulation);
  }
}

void rtl_433_ESP::getStatus(int status)
{
  alogprintfLn(LOG_INFO, " ");
  logprintf(LOG_INFO, "Status Message: Gap length: %lu", signalStart - gapStart);
  alogprintf(LOG_INFO, ", Signal RSSI: %d", signalRssi);
  alogprintf(LOG_INFO, ", train: %d", _actualPulseTrain);
  alogprintf(LOG_INFO, ", messageCount: %d", messageCount);
  alogprintf(LOG_INFO, ", _enabledReceiver: %d", _enabledReceiver);
  alogprintf(LOG_INFO, ", receiveMode: %d", receiveMode);
  alogprintf(LOG_INFO, ", currentRssi: %d", currentRssi);
  alogprintf(LOG_INFO, ", StackHighWaterMark: %d", uxTaskGetStackHighWaterMark(NULL));
  alogprintf(LOG_INFO, ", modulation: %d", modulation);
  alogprintfLn(LOG_INFO, ", pulses: %d", _nrpulses);

  data_t *data;

  /* clang-format off */
  data = data_make(
                "model", "",      DATA_STRING,  "status",
                "protocol", "",   DATA_STRING,  "debug",
                "debug", "",      DATA_INT,     rtlVerbose,
                "duration", "",   DATA_INT,     micros() - signalStart,
                "Gap length", "", DATA_INT,     (signalStart - gapStart),
                "signalRssi", "", DATA_INT,     signalRssi,
                "train", "", DATA_INT,          _actualPulseTrain,
                "messageCount", "", DATA_INT,   messageCount,
                "_enabledReceiver", "", DATA_INT, _enabledReceiver,
                "receiveMode", "", DATA_INT,    receiveMode,
                "currentRssi", "", DATA_INT,    currentRssi,
                "messageCount", "", DATA_INT,   messageCount,
                "pulses", "", DATA_INT,         _nrpulses,
                "StackHighWaterMark", "", DATA_INT, uxTaskGetStackHighWaterMark(NULL),
                "modulation", "",   DATA_INT,     modulation,
                "freeMem", "", DATA_INT,        ESP.getFreeHeap(),
                NULL);
  /* clang-format on */
  r_cfg_t *cfg = &g_cfg;

  data_print_jsons(data, cfg->messageBuffer, cfg->bufferSize);
  (cfg->callback)(cfg->messageBuffer);
  data_free(data);
}
