/* OWN generated exact fixture/header expectations, not codec results. */
#ifndef OWN_MP3_FIXTURE_CASES_H
#define OWN_MP3_FIXTURE_CASES_H
typedef struct {const char*name;unsigned rate,channels,version,bytes,frames,pcmFrames;int positive;} OwnMp3Case;
static const OwnMp3Case own_cases[4]={
{"mono44100",44100,1,1,10031,24,27648,1},
{"stereo44100",44100,2,1,15046,24,27648,1},
{"stereo22050",22050,2,2,5224,25,14400,1},
{"unsupported48000",48000,2,1,14976,26,29952,0},
};
#endif
