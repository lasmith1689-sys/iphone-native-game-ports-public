/* Own positive compressed fixtures; unchanged decoder expectations. */
typedef struct {const char*name;unsigned rate,channels,bytes,frames;} Case;
static const Case cases[3]={
{"mono44100",44100,1,10031,27648},
{"stereo44100",44100,2,15046,27648},
{"stereo22050",22050,2,5224,14400},
};
