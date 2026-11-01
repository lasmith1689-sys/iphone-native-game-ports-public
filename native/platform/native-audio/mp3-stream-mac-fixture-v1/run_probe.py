"""Future OWN Mac-only composition diagnostic. Import performs no actions."""
from pathlib import Path
import hashlib,json,os,platform,struct,sys,time,urllib.request
import bounded_process as bounded
from validate_native import parse
H=Path(__file__).resolve().parent
UNITS=('ngp_audio_shared_implementation_v1.c','ngp_mp3_envelope_v1.c','ngp_voice_service_v1.c','ngp_device_mixer_render_v1.c','fixture_main.c')
DEP={'miniaudio.h':(4099492,'7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6'),'LICENSE':(2597,'457f1b500e0adf6bc059edddfa78a2f62012e7c3bb43476c20e0bd23b25ba0eb')}
def ident(b):return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def compile_argv(clang,sdk,work,label,name):
 obj=work/(label+'.o');warnings=['-Werror=implicit-function-declaration']if name==UNITS[0]else['-Wall','-Wextra','-Werror']
 return [clang,'-target','arm64-apple-macos14.0','-isysroot',sdk,'-std=c11','-O0','-g0','-UNDEBUG','-fno-inline','-fno-lto','-fno-optimize-sibling-calls','-fstack-usage','-MMD','-MF',obj.with_suffix('.d'),'-ferror-limit=20',*warnings,'-I',H,'-I',work,'-c',H/name,'-o',obj]
def header(raw,kind):
 assert type(raw)is bytes and 32<=len(raw)<=8*1024**2;v=struct.unpack_from('<8I',raw);assert v[:4]==(0xfeedfacf,0x100000c,0,kind)and v[7]==0 and 0<v[4]<=128 and 32+v[5]<=len(raw)
 return dict(identity=ident(raw),cpu=v[1],fileType=v[3],loadCommands=v[4],loadCommandBytes=v[5],headerFactsOnly=True,providerABIOrWholeStackAdmission=False)
def main():
 assert sys.platform=='darwin'and platform.machine()=='arm64'
 import resource
 resource.setrlimit(resource.RLIMIT_CORE,(0,0))
 out=H/'probe-output';assert not out.exists();out.mkdir();work=out/'work';work.mkdir();snap=out/'source-snapshot';snap.mkdir();deadline=time.monotonic()+900;commands=[];tools={};errors=[];facts={};hash_bytes=0
 source=json.loads(bounded.read(H/'source-manifest.json',65536))['files'];report=dict(passed=False,MacDirectCompositionObserved=False,compilerResults={},native=[],zeroOracleRefused=False,SDKOrDeviceOrGuestOrMilesOrIncrementalStreamProof=False,productionEncodedHelperAdmitted=False,toolDescendantDrainOrEscapedProcessProof=False)
 def closure():
  for n,w in source.items():assert ident(bounded.read(H/n,262144))==w
 def tool_id(p):
  nonlocal hash_bytes
  before=p.stat();assert p.is_file()and before.st_size<=512*1024**2;h=hashlib.sha256();n=0
  with p.open('rb')as f:
   a=os.fstat(f.fileno());assert(a.st_dev,a.st_ino,a.st_size,a.st_mtime_ns)==(before.st_dev,before.st_ino,before.st_size,before.st_mtime_ns)
   while raw:=f.read(1024**2):n+=len(raw);hash_bytes+=len(raw);assert hash_bytes<=2*1024**3;h.update(raw)
   b=os.fstat(f.fileno());assert(a.st_dev,a.st_ino,a.st_size,a.st_mtime_ns)==(b.st_dev,b.st_ino,b.st_size,b.st_mtime_ns)
  after=p.stat();assert(before.st_dev,before.st_ino,before.st_size,before.st_mtime_ns)==(after.st_dev,after.st_ino,after.st_size,after.st_mtime_ns)and n==before.st_size
  return dict(bytes=n,sha256=h.hexdigest())
 def save():
  for n,v in [('commands.json',commands),('tools.json',tools),('report.json',report),('metadata.json',facts),('errors.json',errors)]:(out/n).write_text(json.dumps(v,sort_keys=True,indent=2)+'\n')
 def command(label,argv,native=False):
  assert Path.cwd()==work and len(commands)<24;closure();p=Path(argv[0]).resolve(strict=True)
  if native:assert p==(work/'own-mp3-stream').resolve(strict=True);header(bounded.read(p,8*1024**2),2)
  key=str(p)
  if key not in tools:tools[key]=dict(before=tool_id(p),after=None)
  commands.append(dict(label=label,argv=list(map(str,argv)),cwd=str(work),executionStartedOrCompleted=False));save()
  row,streams=bounded.execute(label,list(map(str,argv)),work,{k:v for k,v in os.environ.items()if k in('PATH','HOME','TMPDIR')},timeout=30 if native else 60,cap=4096 if native else 65536,total_deadline=deadline)
  row.update(cwd=str(work),driverIdentity=tools[key]['before'],executionStartedOrCompleted=True);commands[-1]=row
  for side,b in streams.items():(work/(label+'.'+side)).write_bytes(b)
  save();assert row['driverReaped']and row['processGroupAbsentObserved']and not row['timedOut']and not row['streamExceeded'];closure();return row['exitCode'],streams['stdout'].decode().strip()
 def required(label,argv):
  code,text=command(label,argv);assert code==0;return text
 def capture():
  files={};total=0
  for p in sorted(out.rglob('*')):
   if p.is_dir():assert not p.is_symlink();continue
   if p.name=='capture-manifest.json':continue
   n=p.relative_to(out).as_posix();assert len(files)<160 and '..'not in n.split('/')and '.git'not in n.split('/');b=bounded.read(p,8*1024**2);total+=len(b);assert total<=64*1024**2;files[n]=ident(b)
  context={k:os.environ[k]for k in ['GITHUB_REPOSITORY','GITHUB_REF','GITHUB_SHA','GITHUB_RUN_ID','GITHUB_RUN_ATTEMPT','GITHUB_JOB']if k in os.environ}
  (out/'capture-manifest.json').write_text(json.dumps(dict(files=files,totalBytes=total,context=context,sourceManifest=ident((H/'source-manifest.json').read_bytes()),SDKBodiesCopied=False,deviceOrGuestOrProductionHelperRun=False),sort_keys=True,indent=2)+'\n')
 old=Path.cwd();os.chdir(work)
 try:
  closure()
  for n in source:p=snap/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(bounded.read(H/n,262144))
  (snap/'source-manifest.json').write_bytes(bounded.read(H/'source-manifest.json',65536))
  for n,(size,sha)in DEP.items():
   with urllib.request.urlopen('https://raw.githubusercontent.com/mackron/miniaudio/f40cf03f80cdb7e741d43e53b7e706e8c1394bcf/'+n,timeout=30)as f:raw=f.read(size+1)
   assert ident(raw)==dict(bytes=size,sha256=sha);(work/n).write_bytes(raw)
  developer=Path(required('developer',['/usr/bin/xcode-select','-p'])).resolve(strict=True)
  advertised=Path(required('macosx-path',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path']));version=required('macosx-version',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-version']);sdk=advertised.resolve(strict=True)
  report.update(SDKAdvertisedPath=str(advertised),SDKPath=str(sdk),SDKVersion=version);assert advertised.is_absolute()and advertised.name=='MacOSX'+version+'.sdk'and int(version.split('.')[0])>=14 and sdk.is_relative_to(developer)and sdk.is_dir()
  clang=Path(required('clang-path',['/usr/bin/xcrun','--sdk','macosx','--find','clang'])).resolve(strict=True);ld=Path(required('ld-path',['/usr/bin/xcrun','--sdk','macosx','--find','ld'])).resolve(strict=True);assert clang.is_relative_to(developer)and ld.is_relative_to(developer)
  for p in(clang,ld):tools.setdefault(str(p),dict(before=tool_id(p),after=None))
  required('clang-version',[clang,'--version']);required('xcode-version',['/usr/bin/xcodebuild','-version'])
  objects=[]
  for i,name in enumerate(UNITS):
   label='mac-'+str(i);obj=work/(label+'.o');code,_=command(label,compile_argv(clang,sdk,work,label,name));report['compilerResults'][label]=code;objects.append(obj)
   if code==0:
    for p in(obj,obj.with_suffix('.su'),obj.with_suffix('.d')):bounded.read(p,8*1024**2)
  exe=work/'own-mp3-stream'
  if all(c==0 for c in report['compilerResults'].values()):report['linkExitCode'],_=command('mac-link',[clang,'-target','arm64-apple-macos14.0','-isysroot',sdk,'-Wl,-fatal_warnings',*objects,'-o',exe])
  for i,obj in enumerate(objects):
   if report['compilerResults']['mac-'+str(i)]==0:facts['mac-'+str(i)]=header(bounded.read(obj,8*1024**2),1)
  if report.get('linkExitCode')==0:
   facts['executable']=header(bounded.read(exe,8*1024**2),2);nonce=os.urandom(16).hex();code,text=command('zero-oracle',[exe,'zero-oracle',nonce],True);report['zeroOracleExitCode']=code
   report['zeroOracleRefused']=code==91 and text==''and(work/'zero-oracle.stderr').read_bytes()==b'own STREAM zero oracle refused\n'
   if report['zeroOracleRefused']:
    for case in json.loads(bounded.read(H/'cases.json',65536)):
     nonce=os.urandom(16).hex();label='stream-'+case['name'];code,text=command(label,[exe,case['name'],H/case['path'],nonce],True);row=dict(case=case['name'],nonce=nonce,exitCode=code,passed=False);report['native'].append(row)
     if code==0 and commands[-1]['stderr']['fileBytes']==0:
      try:row['observed']=parse(text,case,nonce);row['passed']=True
      except Exception as e:errors.append(dict(stage=label,error=type(e).__name__,detail=str(e)[:512]))
  report['MacDirectCompositionObserved']=report['zeroOracleRefused']and len(report['native'])==3 and all(r['passed']for r in report['native'])
  report['passed']=len(report['compilerResults'])==5 and all(c==0 for c in report['compilerResults'].values())and report.get('linkExitCode')==0 and report['MacDirectCompositionObserved']and not errors
 except Exception as e:errors.append(dict(stage='runner',error=type(e).__name__,detail=str(e)[:512]));report['passed']=False
 finally:
  try:
   for n,t in tools.items():t['after']=tool_id(Path(n))
   closure();report.update(toolHashBytesRead=hash_bytes,toolHashesStable=all(t['before']==t['after']for t in tools.values()),sourceClosureStable=True);report['passed']=report['passed']and report['toolHashesStable']
  except Exception as e:errors.append(dict(stage='stability',error=type(e).__name__,detail=str(e)[:512]));report['passed']=False
  try:save();capture()
  finally:os.chdir(old)
 assert report['passed'];print(json.dumps(report))
if __name__=='__main__':main()
