#pragma once
#ifndef CONSTANTS_H_
#define CONSTANTS_H_

static constexpr int kBufferSize = 512;

static constexpr int kMaxWavFiles = 4096;
static constexpr int kKdTreeMaxResults = 1;

static constexpr int kSampleRate = 48000;
static constexpr float kNyquist = kSampleRate / 2.0;

static constexpr int kAnalysisSize = 1024;
static constexpr int kPowerSpecSize = kAnalysisSize / 2 + 1;
static constexpr int kOverlap = 2;

static constexpr int kMaxMfccs = kSampleRate * 10 / (kAnalysisSize / kOverlap);

static constexpr const char* kCoordinatesMapFilename = "/cloud.data";

// Must be power of 2
static constexpr int kNumMelBands = 32;

static constexpr float kMelLowHz = 0;
static constexpr float kMelHighHz = kNyquist;

static constexpr float kTopDb = 80.0f;

static constexpr int kFeatureVecLen = 3 * kNumMelBands;

static float kPcaCoeffs[2][96] = {
    {-0.003080435256959797,  0.07553505126623257,    0.22561913551295137,
     0.050744091553379216,   -0.09317522122325733,   -0.11203286295958623,
     -0.13060103780725743,   0.02259964405357101,    0.055424366191154345,
     0.06687498390846358,    0.03779066869449765,    -0.03963237854743973,
     -0.07553349177468266,   -0.150132835976164,     0.0003403853953335978,
     0.0810019727881983,     0.30995717552925395,    0.04002762477450138,
     -0.009474602275452503,  -0.1372334673139405,    -0.216680996369697,
     -0.12466119177772923,   0.011119946807562774,   0.13386277651339315,
     0.07862452444660852,    0.01062166508353478,    -0.05090375525224486,
     -0.1037214311549366,    -0.054418601695887546,  0.004989349636222051,
     0.10154871249581005,    0.1339156756342234,     0.11693009912924222,
     -0.01567456532865737,   -0.23536131222242507,   -0.08798410610276698,
     -0.0025558080300803233, 0.03301189578819504,    0.1267720109568412,
     0.06406755446403031,    0.0911500297724807,     -0.07014849018004653,
     -0.07891727736363452,   -0.09075524958519852,   0.01222911342749159,
     0.14803241074041046,    0.0902789598490443,     0.06642657147093563,
     -0.044189859415878115,  -0.056814831574246466,  -0.10505726310201734,
     -0.02045626353692783,   0.10512463358284153,    0.27147333073538693,
     0.06294837242348637,    -0.0015220798724243656, -0.10531747326334427,
     -0.07695014652077505,   -0.06357490640100444,   0.017329395707811057,
     0.06304131738188724,    0.09214526804896822,    0.01076874632973046,
     -0.0776900017829354,    -0.0873182087811025,    -0.0638478311179702,
     0.007226345819804663,   0.03883356158050402,    0.07874100639716486,
     0.06844984138860381,    -0.004297794353336839,  -0.0745680139078919,
     -0.1488631202578474,    -0.018038758254605075,  0.04011749654963315,
     0.14055524906179206,    0.05395566449724425,    0.024983050217787017,
     -0.10338501141702011,   -0.17236377396482477,   -0.1954889879603179,
     -0.0017494199034436796, 0.17565378953596855,    0.12257265625020602,
     0.0721343793407364,     -0.07904330067907116,   -0.06931428023055831,
     -0.11407240181961739,   -0.005075272593185072,  0.07860600900645569,
     0.13355185652499107,    0.10296321056892131,    -0.008081828463925366,
     -0.1662043082864419,    -0.0934442466929155,    -0.027233759403798522},
    {0.0817332479686941,    0.03442413660904631,   -0.015044510228762964,
     -0.16342453702610993,  -0.12092082904141499,  -0.016222392732981317,
     0.13189793431596872,   0.09499001830268396,   0.05456657035881685,
     -0.025571608518270394, -0.07252707835591995,  -0.07509767926624922,
     -0.043800900503585066, 0.0676075744093117,    0.1248434847045296,
     0.33318947877956523,   -0.06507628374799371,  -0.04476210037504726,
     -0.11555843025138057,  -0.13604441796812233,  -0.04060424986709033,
     0.11744387953964704,   0.12458776216393087,   0.07742962786581481,
     -0.01590255830731927,  -0.10499584887029365,  -0.025686483805571703,
     0.007247905341443438,  0.07328455687647278,   0.09567938162749183,
     0.09193389239066571,   -0.002688613791178436, -0.11295808797578807,
     -0.11343598188341816,  -0.0957783062832036,   0.07512035083009352,
     0.06848820221195313,   0.0988257745222768,    0.010467042028106959,
     -0.06562291922662483,  -0.2889105350007005,   -0.0807497590489249,
     -0.03980083526032191,  0.08578682584139469,   0.09338943954255222,
     0.11652662401390575,   -0.012103383019044633, -0.15055990294912033,
     -0.04743308681161969,  -0.07010915158657499,  0.05243993387571805,
     0.11375862597589695,   0.17723134394820275,   -0.038482818834887945,
     -0.0828371175775669,   -0.17125584595030435,  -0.08696703286037964,
     0.02795298690833816,   0.0642142494962842,    0.05582182487654279,
     0.02207129458015249,   -0.013166342894667117, -0.09865447559991582,
     -0.061569797276447284, -0.01796412651620666,  0.06265574840158805,
     0.06087425563785812,   0.052875599650574055,  0.00929534243041756,
     -0.09060646669094993,  -0.1605350928171374,   -0.037767889469154846,
     0.14783015799779706,   0.10855233094871071,   0.1890738703696096,
     -0.010398455784103963, -0.04920261434106834,  -0.2031486711236139,
     -0.12402210811608391,  -0.09734062779108292,  0.1471364731904885,
     0.18054855511951737,   0.07917744418015138,   -0.0007776559542527836,
     -0.1247799159128729,   -0.06105850544487979,  -0.04563741679242678,
     0.1034538965406898,    0.11182321090243852,   0.09854140395575334,
     -0.02323213617998377,  -0.09424497729529845,  -0.1009042641949381,
     -0.13538706685124866,  0.04520617795066592,   0.04573576029632961},
};

#endif  // CONSTANTS_H_