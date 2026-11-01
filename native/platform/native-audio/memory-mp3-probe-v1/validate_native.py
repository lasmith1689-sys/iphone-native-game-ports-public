"""Exact future OWN silence result schema; never fabricates observed fields."""
def validate(r):
 expected={'format','successfulDecodes','decodedPCMFrames','zeroPCMSampleChecks','checks','mainDecodeCode','mainViewCode','mainReleaseCode','foreignViewCode','foreignReleaseCode','badReservoirCode','slot65Code','silenceOnly','bitstreamIntegrityCertified','gaplessOrMusicOrStock53Proved','deviceAudioOrGameOrPhoneExecuted'}
 assert type(r)is dict and set(r)==expected and r['format']=='own-MP3-silence-smoke-v1'
 for k,v in [('successfulDecodes',75),('decodedPCMFrames',92736),('zeroPCMSampleChecks',182016),('mainDecodeCode',6),('mainViewCode',6),('mainReleaseCode',6),('foreignViewCode',6),('foreignReleaseCode',6),('badReservoirCode',5),('slot65Code',3)]:assert type(r[k])is int and r[k]==v
 assert type(r['checks'])is int and 182391<=r['checks']<=250000
 assert r['silenceOnly']is True
 for k in ['bitstreamIntegrityCertified','gaplessOrMusicOrStock53Proved','deviceAudioOrGameOrPhoneExecuted']:assert r[k]is False
 return r
