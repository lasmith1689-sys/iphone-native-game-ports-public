"""Future own-process runner. Import/tests never start a process."""
from pathlib import Path
import hashlib,os,signal,stat,subprocess,time
class ProcessError(ValueError):pass
def need(v,m):
 if not v:raise ProcessError(m)
def identity(b):return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def signature(s):
 base=(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns)
 # Python3.14 Windows lstat/fstat expose different ctime interpretations.
 # Future build runner requires Darwin, where the full ctime guard applies.
 return base if os.name=='nt' else base+(s.st_ctime_ns,)
def read(path,cap):
 s=path.lstat();need(stat.S_ISREG(s.st_mode) and not path.is_symlink() and not getattr(s,'st_file_attributes',0)&1024 and s.st_nlink==1 and s.st_size<=cap,'regular bounded single-link input')
 with path.open('rb') as f:
  need(signature(os.fstat(f.fileno()))==signature(s),'descriptor identity');b=f.read(cap+1);need(signature(os.fstat(f.fileno()))==signature(s),'stable descriptor')
 need(len(b)==s.st_size and signature(path.lstat())==signature(s),'stable path');return b
def group_absent(pid):
 try:os.killpg(pid,0)
 except ProcessLookupError:return True
 except PermissionError:return False
 return False
def execute(label,argv,work,env,*,timeout=60,cap=262144,total_deadline):
 need(type(argv) is list and argv and all(type(a) is str and 0<len(a)<=4096 and '\0' not in a for a in argv),'argv bound');need(type(timeout) is int and 1<=timeout<=60 and type(cap) is int and 1<=cap<=262144,'fixed process budget');need(time.monotonic()<total_deadline,'overall deadline')
 stdout=work/(label+'.stdout');stderr=work/(label+'.stderr');need(not stdout.exists() and not stderr.exists(),'fresh fixed streams');started=time.monotonic();timed=False;over=False;killed=False;kill_attempted=False;p=None;rc=None;absent=None;first=None;cleanup=[];spawn_attempted=False
 def failure(stage,error):
  nonlocal first
  row=dict(stage=stage,errorClass=type(error).__name__,message=str(error)[:512])
  if first is None:first=row
  else:cleanup.append(row)
 def signal_once():
  nonlocal killed,kill_attempted
  if p is None or kill_attempted:return
  kill_attempted=True # One attempt, never retry a failed signal or command.
  try:os.killpg(p.pid,signal.SIGKILL);killed=True
  except ProcessLookupError:pass
  except BaseException as e:failure('signal-owned-group',e)
 try:
  with stdout.open('xb')as a,stderr.open('xb')as b:
   spawn_attempted=True;p=subprocess.Popen(argv,stdout=a,stderr=b,env=env,start_new_session=True)
   try:
    while p.poll()is None:
     timed=time.monotonic()-started>timeout or time.monotonic()>total_deadline;over=stdout.stat().st_size>cap or stderr.stat().st_size>cap
     if timed or over:failure('observed-deadline-or-soft-stream-cap',ProcessError('deadline or observed soft disk cap exceeded'));signal_once();break
     time.sleep(.02)
    rc=p.wait(timeout=max(.001,min(5,total_deadline-time.monotonic())))
   except BaseException as e:failure('wait-or-monitor-owned-driver',e)
   finally:
    # Every exceptional path with a real returned Popen object retires only
    # that own session/group. A constructor failure with no object is unknown.
    try:live=p.poll()is None
    except BaseException as e:failure('poll-during-retirement',e);live=True
    try:absent=group_absent(p.pid)
    except BaseException as e:failure('group-query',e);absent=None
    if not live and absent is False:failure('live-owned-group-after-driver',ProcessError('own group remained after driver retirement'))
    if live or absent is not True:signal_once()
    if live:
     try:rc=p.wait(timeout=max(.001,min(5,total_deadline-time.monotonic())))
     except BaseException as e:failure('final-wait-owned-driver',e)
    until=min(time.monotonic()+2,total_deadline)
    try:
     while time.monotonic()<until and not group_absent(p.pid):time.sleep(.02)
     absent=group_absent(p.pid)
    except BaseException as e:failure('final-group-query',e);absent=None
 except BaseException as e:failure('open-stream-or-Popen',e)
 streams={};bodies={}
 for name,path in(('stdout',stdout),('stderr',stderr)):
  try:
   size=path.stat().st_size;over=over or size>cap
   with path.open('rb')as f:body=f.read(cap)
   bodies[name]=body;streams[name]=dict(fileBytes=size,retainedPrefixBytes=len(body),sha256=hashlib.sha256(body).hexdigest(),truncated=size>len(body))
  except BaseException as e:failure('read-retired-stream-'+name,e);bodies[name]=b'';streams[name]=dict(readUncertain=True,errorClass=type(e).__name__,retainedPrefixBytes=0)
 reaped=p is not None and p.returncode is not None
 row=dict(label=label,argv=argv,exitCode=rc,timedOut=timed,streamExceeded=over,driverReaped=reaped,ownSessionStarted=p is not None,spawnOutcomeUncertain=spawn_attempted and p is None,processGroupAbsentObserved=absent,killAttempted=kill_attempted,killSent=killed,firstFailure=first,additionalCleanupFailures=cleanup,retirementUncertain=not reaped or absent is not True,observedSoftDiskCapBytes=cap,retainedPrefixCapBytes=cap,hardDiskWriteCapProved=False,escapedDescendantsProvedAbsent=False,toolDescendantDrainProved=False,elapsedSeconds=time.monotonic()-started,**streams)
 return row,bodies
