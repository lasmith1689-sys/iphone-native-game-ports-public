typedef struct {const char*name;unsigned rate,channels,bytes,frames,kind;} Case;
static const Case cases[5]={
{"mono44100",44100,1,10031,27648,2},
{"stereo44100",44100,2,15046,27648,2},
{"stereo22050",22050,2,5224,14400,2},
{"wave8",22050,1,8262,8192,1},
{"wave16",44100,2,65580,16384,1},
};
