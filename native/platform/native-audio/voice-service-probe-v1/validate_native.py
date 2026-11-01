"""Validate future observed OWN service report, never generate native evidence."""
import math,json
def parse(text,rate,channels,nonce):
 assert type(text)is str and len(text.encode())<=4096
 def pairs(rows):
  d={}
  for k,v in rows:assert k not in d;d[k]=v
  return d
 return validate(json.loads(text,object_pairs_hook=pairs),rate,channels,nonce)
def validate(r,rate,channels,nonce):
 expected={'nonce','rate','channels','acquired','lastInitialHandle','reacquiredHandle','callbackRequests','slowSourceDelta','fastSourceDelta','copyMean','u8Mean','s16Mean','nearEnergy','farEnergy','renderSequence','allocatorViolations','retainedPCMBytes','retainedEngineBytes','terminalStatus','terminalPhase','workerStillParked','realService','AppleAudioOrGameExecuted'}
 assert type(r)is dict and set(r)==expected and type(r['nonce'])is str and r['nonce']==nonce and type(r['rate'])is int and type(r['channels'])is int and r['rate']==rate and r['channels']==channels
 assert r['acquired']==[32,32,13]and all(type(x)is int for x in r['acquired'])
 for k in('lastInitialHandle','reacquiredHandle','callbackRequests','slowSourceDelta','fastSourceDelta','renderSequence','allocatorViolations','retainedPCMBytes','retainedEngineBytes','terminalStatus','terminalPhase'):assert type(r[k])is int and 0<=r[k]<(1<<64)
 assert r['lastInitialHandle']==77 and r['reacquiredHandle']>r['lastInitialHandle']and 10<=r['callbackRequests']<=128 and 0<r['slowSourceDelta']<r['fastSourceDelta']<16384 and r['fastSourceDelta']>2*r['slowSourceDelta']
 for k in('copyMean','u8Mean','s16Mean','nearEnergy','farEnergy'):assert type(r[k])in(int,float)and math.isfinite(r[k])
 assert abs(r['copyMean']-0.125)<0.005 and abs(r['u8Mean']-0.5)<0.01 and abs(r['s16Mean']-0.25)<0.01 and 0<=r['farEnergy']<r['nearEnergy']*0.2
 assert r['renderSequence']>0 and r['allocatorViolations']==0 and 0<r['retainedPCMBytes']<=134217728 and 0<r['retainedEngineBytes']<=8388608 and r['terminalStatus']==14 and r['terminalPhase']==5
 assert r['workerStillParked']is True and r['realService']is True and r['AppleAudioOrGameExecuted']is False
 return r
