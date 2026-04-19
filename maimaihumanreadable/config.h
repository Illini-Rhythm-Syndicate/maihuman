class touchblock
{
public:
  uint8_t mprid;
  uint8_t portid;
  int8_t thresOffset;
};

const uint8_t MPR_PADNUMS[] = {12, 12, 12, 12};
const uint8_t THRESHOLD = 32;
const uint8_t THRES_MIN = 8;

const uint16_t MOVE_BASELINE_THRESHOLD = 150;
const uint16_t THRES_CALIB_DEFAULT = 750;


#define A_GLOBAL_OFFSET -2
#define B_GLOBAL_OFFSET -5
#define C_GLOBAL_OFFSET -16
#define D_GLOBAL_OFFSET -2
#define E_GLOBAL_OFFSET -9

touchblock touchmap[34] = {
 // Group A
    {2, 4, A_GLOBAL_OFFSET}, // 1
    {3, 5, A_GLOBAL_OFFSET}, // 2
    {3, 4, A_GLOBAL_OFFSET}, // 3
    {1, 5, A_GLOBAL_OFFSET}, // 4
    {1, 4, A_GLOBAL_OFFSET}, // 5
    {0, 5, A_GLOBAL_OFFSET}, // 6
    {0, 4, A_GLOBAL_OFFSET}, // 7
    {2, 5, A_GLOBAL_OFFSET}, // 8
    // Group B
    {2, 3, B_GLOBAL_OFFSET}, // 1
    {3, 6, B_GLOBAL_OFFSET}, // 2
    {3, 3, B_GLOBAL_OFFSET - 5}, // 3
    {1, 6, B_GLOBAL_OFFSET}, // 4
    {1, 3, B_GLOBAL_OFFSET}, // 5
    {0, 6, B_GLOBAL_OFFSET + 5}, // 6
    {0, 3, B_GLOBAL_OFFSET}, // 7
    {2, 6, B_GLOBAL_OFFSET - 5}, // 8
    // Group C
    {3, 9, C_GLOBAL_OFFSET}, // 1
    {0, 9, C_GLOBAL_OFFSET - 2}, // 2
    // Group D
    {2, 7, D_GLOBAL_OFFSET}, // 1
    {2, 2, D_GLOBAL_OFFSET}, // 2
    {3, 7, D_GLOBAL_OFFSET + 5}, // 3
    {3, 2, D_GLOBAL_OFFSET + 0}, // 4
    {1, 7, D_GLOBAL_OFFSET}, // 5
    {1, 2, D_GLOBAL_OFFSET}, // 6
    {0, 7, D_GLOBAL_OFFSET - 2}, // 7
    {0, 2, D_GLOBAL_OFFSET - 2}, // 8
    // Group E
    {2, 8, E_GLOBAL_OFFSET}, // 1
    {2, 1, E_GLOBAL_OFFSET}, // 2
    {3, 8, E_GLOBAL_OFFSET - 2}, // 3
    {3, 1, E_GLOBAL_OFFSET + 0}, // 4
    {1, 8, E_GLOBAL_OFFSET - 2}, // 5
    {1, 1, E_GLOBAL_OFFSET - 2}, // 6
    {0, 8, E_GLOBAL_OFFSET}, // 7
    {0, 1, E_GLOBAL_OFFSET + 0}, // 8
};

// the release threashold will be set to THRESHOLD - RELEASE_THRESHOLD_OFFSET
const uint8_t RELEASE_THRESHOLD_OFFSET = 0;

// AN3944 recommended values are:
// MHDR = 1
// NHDR = 1
// NCLR = 0
// FDLR = 0
// MHDF = 1
// NHDF = 1
// NCLF = 255
// FDLF = 2
// dk values are:
// MHDR = 1
// NHDR = 16
// NCLR = 4
// FDLR = 0
// MHDF = 4
// NHDF = 1
// NCLF = 16
// FDLF = 4
// if some regions get stuck easily, you can try the following values
//   to make the system more sensitive to environmental changes
// MHDR = 2;
// NHDR = 2;
// NCLR = 4;
// FDLR = 0;
// MHDF = 4;
// NHDF = 2;
// NCLF = 16;
// FDLF = 2;
// rising NHD is larger than falling NHD because rising is the release direction
//   the rising changes are more likely to be environmental changes
// rising NCL being smaller than falling NCL is because of the same reason
// the user can experiment with different values to find the best one
//   but those values should be good for most cases
const uint8_t MHDR = 2;
const uint8_t NHDR = 2;
const uint8_t NCLR = 4;
const uint8_t FDLR = 0;
const uint8_t MHDF = 4;
const uint8_t NHDF = 2;
const uint8_t NCLF = 16;
const uint8_t FDLF = 2;


// range: 0 - 3
// 0: 6 samples
// 1: 10 samples
// 2: 18 samples
// 3: 34 samples
// aka. first filter averaging samples
// the highest and lowest are tossed
// so theoretically, the less samples, the system is less prone to noise
// the user shouldn't change this value in any case
const uint8_t FIRST_FILTER_ITERATIONS = 0;

// range: 0 - 7
// actual value is 2^(CDT-1) us
// will be overriden by autoconfig, unless SCTS is set to 1
const uint8_t CHARGE_DISCHARGE_TIME = 1;

// range: 0 - 3
// 0: 4 samples
// 1: 6 samples
// 2: 10 samples
// 3: 18 samples
// aka. second filter averaging samples
// this value and the next value determines the response time and second stage filtering
// changing any of them is basically the same
// setting those two values higher will result in a smoother signal but slower response time
// if the user notices the signal is too noisy, they can try to increase this value
const uint8_t SECOND_FILTER_ITERATIONS = 0;

// range: 0 - 7
// 2^ESI ms of first stage samples are taken into account
// all the first stage samples in the period is averaged
// regarding how to set this value, please refer to the comment above
const uint8_t ELECTRODE_SAMPLE_INTERVAL = 0;

// range: 0 - 7
// when DR + 1 samples are lower than the threshold, the electrode is released
// this smoothes out the signal and prevents false release
//   however, a debounce value that's too high might cause releases that are too short to be missed
// if the user notices too many false releases, try to increase this value. but don't set it too high
const uint8_t RELEASE_DEBOUNCE = 0;

// range: 0 - 7
// when DT + 1 samples are higher than the threshold, the electrode is touched
// this smoothes out the signal and prevents false touch
//   however, a debounce value that's too high might cause touches that are too short to be missed
// if the user notices too many false touches, try to increase this value. but don't set it too high
const uint8_t TOUCH_DEBOUNCE = 1;
// ! the touch response time is debounce * SFI * ESI ms
// ! so in the main code we set read interval to SFI * ESI ms

// please refer to AN3889 page 9 for more information
// the user should not change those values
const uint8_t AUTO_CONFIG_RETRY = 0;
const uint8_t AUTO_CONFIG_BVA = 2;
const uint8_t AUTO_CONFIG_ARE = 1;
const uint8_t AUTO_CONFIG_ACE = 1;

// @datasheet page 15 and 18
// setting this to 1 disables CDT search
// which means it will use a fixed CDT and only search for CDC
// this results in a known first stage sample amount
// however, through testing, setting this to 1 will greatly degrade signal quality
// the user should not change this value
const uint8_t SCTS = 0;

// those values are suitable for 3.3V
// for other voltage systems, please refer to AN3889 page 7 through 8
const uint8_t USL = 202;
const uint8_t TL = 182;
const uint8_t LSL = 131;

// please refer to page 16 of the datasheet for more information
// this value should be the same as AUTO_CONFIG_BVA
const uint8_t CALIBRATION_LOCK = 0b10;
