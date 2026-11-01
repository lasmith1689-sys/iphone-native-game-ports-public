"""Future exact nonce/source/lifetime/PCM schema, no invented observations."""
import json,math
FIELDS={'format','nonce','case','sourceRate','sourceChannels','sourceFrames','outputRate','outputChannels','kind','handle','boundFrames','mp3ReleaseCalls','mp3ResultNull','encodedRetiredBeforePlay','voiceReleaseCalls','callbackRequests','cursorPre','cursorPost','renderSequence','allocatorViolations','stopTailEnergy','amplitude','rms','mean','wrong','off','workersParked','deviceGuestMilesIncrementalGaplessIntegrityProof'}
def parse(text,case,nonce):
 assert type(text)is str and len(text.encode())<=4096
 def pairs(rows):
  d={}
  for k,v in rows:assert k not in d;d[k]=v
  return d
 return validate(json.loads(text,object_pairs_hook=pairs),case,nonce)
def validate(r,case,nonce):
 assert type(r)is dict and set(r)==FIELDS and r['format']=='own-MP3-STREAM-v1'and type(r['nonce'])is str and r['nonce']==nonce and r['case']==case['name'];e=case['envelope']
 for k,v in [('sourceRate',e['rate']),('sourceChannels',e['channels']),('sourceFrames',e['pcmFrames']),('outputRate',48000),('outputChannels',2),('kind',3),('boundFrames',e['pcmFrames']),('mp3ReleaseCalls',1),('voiceReleaseCalls',1),('callbackRequests',12),('allocatorViolations',0)]:assert type(r[k])is int and r[k]==v
 for k in ['handle','cursorPre','cursorPost','renderSequence']:assert type(r[k])is int and 0<=r[k]<(1<<64)
 assert r['handle']>0 and 0<=r['cursorPost']<r['cursorPre']<=e['pcmFrames']and r['renderSequence']>=10
 for k in ['mp3ResultNull','encodedRetiredBeforePlay','workersParked']:assert r[k]is True
 assert r['deviceGuestMilesIncrementalGaplessIntegrityProof']is False
 for k in ['stopTailEnergy','amplitude','rms','mean','wrong','off']:assert type(r[k])is list and len(r[k])==2 and all(type(x)in(int,float)and math.isfinite(x)for x in r[k])
 for ch in range(2):assert 0<=r['stopTailEnergy'][ch]<1e-12 and .30<=r['amplitude'][ch]<=.60 and .20<=r['rms'][ch]<=.46 and abs(r['mean'][ch])<=.025 and 0<=r['wrong'][ch]<=.08 and 0<=r['off'][ch]<=.06
 return r
