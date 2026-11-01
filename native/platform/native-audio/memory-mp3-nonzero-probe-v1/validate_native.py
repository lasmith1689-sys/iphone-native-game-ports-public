"""Future exact observed-schema/range validation; never native evidence."""
from pathlib import Path
import json,math
T=json.loads((Path(__file__).parent/'tolerances.json').read_text())
def parse(text,case,nonce):
 assert type(text)is str and len(text.encode())<=4096
 def pairs(rows):
  d={}
  for k,v in rows:assert k not in d;d[k]=v
  return d
 return validate(json.loads(text,object_pairs_hook=pairs),case,nonce)
def validate(r,case,nonce):
 expected={'format','nonce','case','status','rate','channels','decodedFrames','releaseCalls','ownedResultNull','profileRefused','amplitude','rms','mean','wrong','off','skipFirst','skipLast','gaplessOrIntegrityOrMusicCohortOrStock53Proof','AppleAudioOrGameExecuted'}
 assert type(r)is dict and set(r)==expected and r['format']=='own-nonzero-MP3-v1'and type(r['nonce'])is str and r['nonce']==nonce and r['case']==case['name']
 e=case['envelope'];positive=case['positive'];assert type(positive)is bool
 for k,v in [('status',case['expectedDecodeCode']),('rate',e['rate']),('channels',e['channels']),('decodedFrames',e['pcmFrames']if positive else 0),('skipFirst',4096),('skipLast',4096)]:assert type(r[k])is int and r[k]==v
 assert type(r['releaseCalls'])is int and r['releaseCalls']==int(positive)and r['ownedResultNull']is True and r['profileRefused']is(not positive)and r['gaplessOrIntegrityOrMusicCohortOrStock53Proof']is False and r['AppleAudioOrGameExecuted']is False
 for k in ['amplitude','rms','mean','wrong','off']:assert type(r[k])is list and len(r[k])==2 and all(type(x)in(int,float)and math.isfinite(x)for x in r[k])
 for ch in range(2):
  if positive and ch<e['channels']:
   assert T['amplitudeMinimum']<=r['amplitude'][ch]<=T['amplitudeMaximum']and T['rmsMinimum']<=r['rms'][ch]<=T['rmsMaximum']and abs(r['mean'][ch])<=T['absoluteMeanMaximum']and 0<=r['wrong'][ch]<=T['wrongToneAmplitudeMaximum']and 0<=r['off'][ch]<=T['offToneAmplitudeMaximum']
  else:assert all(r[k][ch]==0 for k in ['amplitude','rms','mean','wrong','off'])
 return r
