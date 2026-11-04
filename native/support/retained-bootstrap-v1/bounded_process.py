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
 stdout=work/(label+'.stdout');stderr=work/(label+'.stderr');need(not stdout.exists() and not stderr.exists(),'fresh fixed streams');started=time.monotonic();timed=False;over=False;killed=False
 with stdout.open('xb') as a,stderr.open('xb') as b:
  p=subprocess.Popen(argv,stdout=a,stderr=b,env=env,start_new_session=True)
  while p.poll() is None:
   timed=time.monotonic()-started>timeout or time.monotonic()>total_deadline;over=stdout.stat().st_size>cap or stderr.stat().st_size>cap
   if timed or over:
    try:os.killpg(p.pid,signal.SIGKILL);killed=True
    except ProcessLookupError:pass
    break
   time.sleep(.02)
  try:rc=p.wait(timeout=5)
  except subprocess.TimeoutExpired:
   try:os.killpg(p.pid,signal.SIGKILL);killed=True
   except ProcessLookupError:pass
   rc=p.wait(timeout=5)
 absent=group_absent(p.pid)
 if not absent:
  try:os.killpg(p.pid,signal.SIGKILL);killed=True
  except ProcessLookupError:pass
  until=min(time.monotonic()+2,total_deadline)
  while time.monotonic()<until and not group_absent(p.pid):time.sleep(.02)
  absent=group_absent(p.pid)
 streams={};bodies={}
 for name,path in (('stdout',stdout),('stderr',stderr)):
  size=path.stat().st_size;over=over or size>cap
  with path.open('rb') as f:body=f.read(cap)
  bodies[name]=body;streams[name]=dict(fileBytes=size,retainedPrefixBytes=len(body),sha256=hashlib.sha256(body).hexdigest(),truncated=size>len(body))
 row=dict(label=label,argv=argv,exitCode=rc,timedOut=timed,streamExceeded=over,driverReaped=p.returncode is not None,ownSessionStarted=True,processGroupAbsentObserved=absent,killSent=killed,escapedDescendantsProvedAbsent=False,toolDescendantDrainProved=False,elapsedSeconds=time.monotonic()-started,**streams)
 return row,bodies
