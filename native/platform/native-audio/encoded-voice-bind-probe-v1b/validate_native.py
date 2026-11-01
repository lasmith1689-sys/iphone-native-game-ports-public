"""Future actual helper report schema. Unit oracles never become observations."""
import json,math
BASE={'format','nonce','case','sourceRate','sourceChannels','sourceFrames','outputRate','outputChannels','kind','handle','boundFrames','mp3ReleaseCalls','mp3ResultNull','encodedRetiredBeforePlay','voiceReleaseCalls','callbackRequests','cursorPre','cursorPost','renderSequence','allocatorViolations','stopTailEnergy','amplitude','rms','mean','wrong','off','workersParked','deviceGuestMilesIncrementalGaplessIntegrityProof'}
EXTRA={'formatKind','constantMean','admission','wave','decode','view','bind','release','copyCommitted','releaseConfirmed','helperPoisoned','negativeMP3','negativeWAVE','mainAdmission','foreignAdmission','helperReportBytes'}
def parse(text,case,nonce):
 assert type(text)is str and len(text.encode())<=4096
 def pairs(rows):
  d={}
  for k,v in rows:assert k not in d;d[k]=v
  return d
 r=json.loads(text,object_pairs_hook=pairs);assert type(r)is dict and set(r)==BASE|EXTRA and r['format']=='own-ENCODED-VOICE-v1'and r['nonce']==nonce and r['case']==case['name'];e=case['envelope'];mp3=case['kind']==2;nr=-2147483647
 expected={'sourceRate':e['rate'],'sourceChannels':e['channels'],'sourceFrames':e['pcmFrames'],'outputRate':48000,'outputChannels':2,'kind':3,'boundFrames':e['pcmFrames'],'mp3ReleaseCalls':1 if mp3 else 0,'voiceReleaseCalls':1,'callbackRequests':12,'allocatorViolations':0,'formatKind':case['kind'],'admission':0,'wave':nr if mp3 else 0,'decode':0 if mp3 else nr,'view':0 if mp3 else nr,'bind':0,'release':0 if mp3 else nr,'copyCommitted':1,'releaseConfirmed':1 if mp3 else 0,'helperPoisoned':0,'negativeMP3':2,'negativeWAVE':1,'mainAdmission':2,'foreignAdmission':2}
 for k,v in expected.items():assert type(r[k])is int and r[k]==v
 for k in('handle','cursorPre','cursorPost','renderSequence','helperReportBytes'):assert type(r[k])is int and 0<=r[k]<(1<<64)
 assert r['handle']>0 and 0<=r['cursorPost']<r['cursorPre']<=e['pcmFrames']and r['renderSequence']>=10 and 64<=r['helperReportBytes']<=1024
 assert r['mp3ResultNull']is mp3
 for k in('encodedRetiredBeforePlay','workersParked'):assert r[k]is True
 assert r['deviceGuestMilesIncrementalGaplessIntegrityProof']is False
 for k in('stopTailEnergy','amplitude','rms','mean','wrong','off','constantMean'):assert type(r[k])is list and len(r[k])==2 and all(type(x)in(int,float)and math.isfinite(x)for x in r[k])
 for ch in range(2):
  assert 0<=r['stopTailEnergy'][ch]<1e-12
  if mp3:assert .30<=r['amplitude'][ch]<=.60 and .20<=r['rms'][ch]<=.46 and abs(r['mean'][ch])<=.025 and 0<=r['wrong'][ch]<=.08 and 0<=r['off'][ch]<=.06 and r['constantMean'][ch]==0
  else:
   expected_wave=(.379411876,.284558907)if e['channels']==1 else(.09375,.140625)
   assert abs(r['constantMean'][ch]-expected_wave[ch])<=.000003
   for k in('amplitude','rms','mean','wrong','off'):assert r[k][ch]==0 # Explicit uncalled tone profile, not measured spectral silence.
 return r
