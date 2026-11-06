typedef struct{const char*name;unsigned rate,channels,version,bytes,frames,pcmFrames;int positive;unsigned padding,paddingKind,encodedBytes;}OwnMp3Case;
static const OwnMp3Case own_cases[10]={
{"mono44100",44100,1,1,10031,24,27648,1,0,0,10031},
{"stereo44100",44100,2,1,15046,24,27648,1,0,0,15046},
{"stereo22050",22050,2,2,5224,25,14400,1,0,0,5224},
{"unsupported48000",48000,2,1,14976,26,29952,0,0,0,14976},
{"own-padding-1-1",44100,1,1,10032,24,27648,1,1,1,10031},
{"own-padding-1-522",44100,1,1,10553,24,27648,1,522,1,10031},
{"own-padding-1-4093",44100,1,1,14124,24,27648,1,4093,1,10031},
{"own-padding-2-2",44100,1,1,10033,24,27648,1,2,2,10031},
{"own-padding-2-417",44100,1,1,10448,24,27648,1,417,2,10031},
{"own-padding-2-4093",44100,1,1,14124,24,27648,1,4093,2,10031},
};
