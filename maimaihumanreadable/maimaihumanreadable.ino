// original author: HelloworldDk
// modified for human-readable live Python streaming with remote reset

#define SerialDevice Serial
#include "Adafruit_MPR121.h"
#include "config.h"

Adafruit_MPR121 mpr[4];
uint32_t lastMillis = 0;
uint8_t DATA_READ_INTERVAL;

const char* sensorNames[34] = {
  "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8",
  "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8",
  "C1", "C2",
  "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8",
  "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8"
};

void MprSetup(Adafruit_MPR121 &cap);
void PrintSensorData();
void InitSensors();

void setup()
{
  SerialDevice.begin(250000);
  SerialDevice.setTimeout(0);
  mpr[0].begin(0x5A, &Wire);
  mpr[1].begin(0x5B, &Wire);
  mpr[2].begin(0x5C, &Wire);
  mpr[3].begin(0x5D, &Wire);
  Wire.setClock(100000);

  InitSensors();

  uint8_t SECOND_FILTER_ITERATION_SAMPLES;
  uint8_t ELECTRODE_SAMPLE_INTERVAL_MS;

  switch (SECOND_FILTER_ITERATIONS)
  {
    case 0: SECOND_FILTER_ITERATION_SAMPLES = 4; break;
    case 1: SECOND_FILTER_ITERATION_SAMPLES = 6; break;
    case 2: SECOND_FILTER_ITERATION_SAMPLES = 10; break;
    case 3: SECOND_FILTER_ITERATION_SAMPLES = 18; break;
  }

  ELECTRODE_SAMPLE_INTERVAL_MS = 1 << ELECTRODE_SAMPLE_INTERVAL;
  DATA_READ_INTERVAL = SECOND_FILTER_ITERATION_SAMPLES * ELECTRODE_SAMPLE_INTERVAL_MS;
}

void loop()
{
  // Check for incoming serial commands
  while (SerialDevice.available() > 0) {
    char c = SerialDevice.read();
    if (c == 'R' || c == 'r') {
      InitSensors();
    }
  }

  uint32_t currentMillis = millis();
  if (currentMillis - lastMillis > DATA_READ_INTERVAL)
  {
    lastMillis = currentMillis;
    PrintSensorData();
  }
}

void InitSensors()
{
  MprSetup(mpr[0]);
  MprSetup(mpr[1]);
  MprSetup(mpr[2]);
  MprSetup(mpr[3]);

  for (uint8_t i = 0; i < 34; i++)
  {
    uint8_t mprid = touchmap[i].mprid;
    uint8_t portid = touchmap[i].portid;
    int8_t thresOffset = touchmap[i].thresOffset;
    
    mpr[mprid].writeRegister(MPR121_TOUCHTH_0 + portid * 2, THRESHOLD + thresOffset);
    mpr[mprid].writeRegister(MPR121_RELEASETH_0 + portid * 2, THRESHOLD - RELEASE_THRESHOLD_OFFSET + thresOffset);
  }

  mpr[0].writeRegister(MPR121_ECR, CALIBRATION_LOCK << 6 | MPR_PADNUMS[0]);
  mpr[1].writeRegister(MPR121_ECR, CALIBRATION_LOCK << 6 | MPR_PADNUMS[1]);
  mpr[2].writeRegister(MPR121_ECR, CALIBRATION_LOCK << 6 | MPR_PADNUMS[2]);
  mpr[3].writeRegister(MPR121_ECR, CALIBRATION_LOCK << 6 | MPR_PADNUMS[3]);
}

void ResetSingleChip(uint8_t target_mprid)
{
  MprSetup(mpr[target_mprid]);

  // Re-apply specific thresholds from touchmap for just this chip
  for (uint8_t i = 0; i < 34; i++)
  {
    if (touchmap[i].mprid == target_mprid) {
      uint8_t portid = touchmap[i].portid;
      int8_t thresOffset = touchmap[i].thresOffset;
      
      mpr[target_mprid].writeRegister(MPR121_TOUCHTH_0 + portid * 2, THRESHOLD + thresOffset);
      mpr[target_mprid].writeRegister(MPR121_RELEASETH_0 + portid * 2, THRESHOLD - RELEASE_THRESHOLD_OFFSET + thresOffset);
    }
  }

  // Re-enable electrodes for just this chip
  mpr[target_mprid].writeRegister(MPR121_ECR, CALIBRATION_LOCK << 6 | MPR_PADNUMS[target_mprid]);
}

void PrintSensorData()
{
  for (uint8_t i = 0; i < 34; i++)
  {
    uint8_t mprid = touchmap[i].mprid;
    uint8_t portid = touchmap[i].portid;

    uint16_t fd_lsb = mpr[mprid].readRegister8(0x04 + portid * 2); 
    uint16_t fd_msb = mpr[mprid].readRegister8(0x05 + portid * 2);
    uint16_t fd = (fd_msb << 8) | fd_lsb;

    uint8_t bv_raw = mpr[mprid].readRegister8(0x1E + portid);
    uint16_t bv = bv_raw << 2;

    // --- SOFTWARE WATCHDOG ---
    int16_t delta = (int16_t)bv - (int16_t)fd;
    if (delta > MOVE_BASELINE_THRESHOLD) {
      // 1. Read AUTOCONFIG0 (0x7B)
      uint8_t autoCfg0 = mpr[mprid].readRegister8(MPR121_AUTOCONFIG0);
      
      // 2. Disable Auto-Config (ACE, bit 0) and Auto-Reconfig (ARE, bit 1)
      mpr[mprid].writeRegister(MPR121_AUTOCONFIG0, autoCfg0 & 0xFC); 
      
      // 3. Put the MPR121 into Stop Mode to unlock register writes
      mpr[mprid].writeRegister(MPR121_ECR, 0x00);
      
      // 4. Write the new baseline
      mpr[mprid].writeRegister(0x1E + portid, (uint8_t)(fd >> 2));
      
      // 5. Calculate scaled thresholds based on config.h offsets
      uint8_t base_tt = THRESHOLD + touchmap[i].thresOffset;
      uint8_t base_rt = THRESHOLD - RELEASE_THRESHOLD_OFFSET + touchmap[i].thresOffset;
      
      // Use 32-bit math to prevent overflow during multiplication, then clamp to at least 1 
      // to prevent the threshold from becoming 0 and causing a permanent lock.
      uint8_t new_tt = max((uint8_t)THRES_MIN, (uint8_t)(((uint32_t)base_tt * fd) / THRES_CALIB_DEFAULT));
      uint8_t new_rt = max((uint8_t)THRES_MIN, (uint8_t)(((uint32_t)base_rt * fd) / THRES_CALIB_DEFAULT));
      
      // Write the new thresholds (Registers 0x41 and 0x42 base)
      mpr[mprid].writeRegister(MPR121_TOUCHTH_0 + portid * 2, new_tt);
      mpr[mprid].writeRegister(MPR121_RELEASETH_0 + portid * 2, new_rt);
      
      // 6. Restore Run Mode, force CL = 0b00 to protect other channels
      uint8_t runModeECR = MPR_PADNUMS[mprid]; 
      mpr[mprid].writeRegister(MPR121_ECR, runModeECR);
    }

    uint8_t tt = mpr[mprid].readRegister8(0x41 + portid * 2);
    uint8_t rt = mpr[mprid].readRegister8(0x42 + portid * 2);

    SerialDevice.print(sensorNames[i]);
    SerialDevice.print(F(":"));
    SerialDevice.print(fd);
    SerialDevice.print(F(","));
    SerialDevice.print(bv);
    SerialDevice.print(F(","));
    SerialDevice.print(tt);
    SerialDevice.print(F(","));
    SerialDevice.print(rt);
    
    if (i < 33) SerialDevice.print(F(" "));
  }
  SerialDevice.println();
}

void MprSetup(Adafruit_MPR121 &cap)
{
  // Soft reset to put the device into POR state
  cap.writeRegister(MPR121_SOFTRESET, 0x63);
  delay(1);
  cap.writeRegister(MPR121_ECR, 0x0);
  
  cap.writeRegister(MPR121_MHDR, MHDR);
  cap.writeRegister(MPR121_NHDR, NHDR);
  cap.writeRegister(MPR121_NCLR, NCLR);
  cap.writeRegister(MPR121_FDLR, FDLR);
  cap.writeRegister(MPR121_MHDF, MHDF);
  cap.writeRegister(MPR121_NHDF, NHDF);
  cap.writeRegister(MPR121_NCLF, NCLF);
  cap.writeRegister(MPR121_FDLF, FDLF);

  //cap.writeRegister(0x33, 4); // NHDT: Walk down 4 units per interval
  //cap.writeRegister(0x34, 2); // NCLT: Only require 2 samples to confirm
  //cap.writeRegister(0x35, 2); // FDLT: Filter delay limit
  
  cap.writeRegister(MPR121_DEBOUNCE, RELEASE_DEBOUNCE << 4 | TOUCH_DEBOUNCE);
  cap.writeRegister(MPR121_CONFIG1, FIRST_FILTER_ITERATIONS << 6);
  cap.writeRegister(MPR121_CONFIG2, CHARGE_DISCHARGE_TIME << 5 | SECOND_FILTER_ITERATIONS << 3 | ELECTRODE_SAMPLE_INTERVAL);
  cap.writeRegister(MPR121_AUTOCONFIG0, FIRST_FILTER_ITERATIONS << 6 | AUTO_CONFIG_RETRY << 4 | AUTO_CONFIG_BVA << 2 | AUTO_CONFIG_ARE << 1 | AUTO_CONFIG_ACE);
  cap.writeRegister(MPR121_AUTOCONFIG1, SCTS << 7);
  
  cap.writeRegister(MPR121_UPLIMIT, USL);
  cap.writeRegister(MPR121_TARGETLIMIT, TL);
  cap.writeRegister(MPR121_LOWLIMIT, LSL);
}