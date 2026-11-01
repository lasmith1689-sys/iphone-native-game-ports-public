"""Future OWN silence-codec/Mac + iPhone compile diagnostic. Import is inert."""
from pathlib import Path
import hashlib,json,os,platform,sys,time,urllib.request
import bounded_process as bounded
from inspect_image import inspect
from validate_native import parse
H=Path(__file__).resolve().parent
REV='f40cf03f80cdb7e741d43e53b7e706e8c1394bcf'
DEPENDENCIES={'miniaudio.h':(4099492,'7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6'),'LICENSE':(2597,'457f1b500e0adf6bc059edddfa78a2f62012e7c3bb43476c20e0bd23b25ba0eb')}
PRODUCTION=('ngp_audio_shared_implementation_v1.c','ngp_mp3_envelope_v1.c')
UNITS=PRODUCTION+('fixture_main.c',)
def ident(raw):return dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())
def sdk_alias(advertised,version,developer):
 sdk=advertised.resolve(strict=True);assert advertised.is_absolute()and sdk.is_relative_to(developer)and sdk.is_dir()
 if advertised.name.startswith('iPhoneOS'):assert version=='27.1'and advertised.name=='iPhoneOS27.1.sdk'
 else:assert int(version.split('.')[0])>=14 and advertised.name=='MacOSX'+version+'.sdk'
 return sdk

def main():
 assert sys.platform=='darwin'and platform.machine()=='arm64'
 import resource
 resource.setrlimit(resource.RLIMIT_CORE,(0,0))
 out=H/'probe-output';assert not out.exists();out.mkdir();work=out/'work';work.mkdir();snap=out/'source-snapshot';snap.mkdir();commands=[];tools={};facts={};errors=[];deadline=time.monotonic()+900;hash_bytes=0
 report=dict(passed=False,MacNonzeroCodecFixture=False,iPhoneCompileLinkStructuralProof=False,codecIntegrityMusicCohortGaplessOrStock53Proved=False,gameOrAppleAudioOrPhoneExecuted=False,toolDescendantDrainOrEscapedProcessProof=False)
 source=json.loads(bounded.read(H/'source-manifest.json',65536))['files']
 def closure():
  for n,w in source.items():assert ident(bounded.read(H/n,262144))==w
 def tool_identity(path):
  nonlocal hash_bytes
  s=path.stat();assert path.is_file()and s.st_size<=512*1024**2;h=hashlib.sha256();n=0
  with path.open('rb')as f:
   before=os.fstat(f.fileno());assert(before.st_dev,before.st_ino,before.st_size,before.st_mtime_ns)==(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns)
   while True:
    raw=f.read(1024**2)
    if not raw:break
    n+=len(raw);hash_bytes+=len(raw);assert n<=512*1024**2 and hash_bytes<=2*1024**3;h.update(raw)
   after=os.fstat(f.fileno());assert(before.st_dev,before.st_ino,before.st_size,before.st_mtime_ns)==(after.st_dev,after.st_ino,after.st_size,after.st_mtime_ns)
  t=path.stat();assert(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns)==(t.st_dev,t.st_ino,t.st_size,t.st_mtime_ns)and n==s.st_size
  return dict(bytes=n,sha256=h.hexdigest())
 def save():
  for n,value in [('report.json',report),('commands.json',commands),('tools.json',tools),('metadata.json',facts),('errors.json',errors)]:(out/n).write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
 def command(label,argv,child=False):
  assert Path.cwd()==work;closure();p=Path(argv[0]).resolve(strict=True);key=str(p)
  if child:
   raw=bounded.read(p,8*1024**2);assert p==(work/'own-mp3-nonzero').resolve(strict=True)and len(raw)>=32 and int.from_bytes(raw[:4],'little')==0xfeedfacf and int.from_bytes(raw[4:8],'little')==0x100000c and int.from_bytes(raw[12:16],'little')==2
  if key not in tools:tools[key]=dict(before=tool_identity(p),after=None)
  commands.append(dict(label=label,argv=list(map(str,argv)),cwd=str(work),executionStartedOrCompleted=False));save()
  row,streams=bounded.execute(label,list(map(str,argv)),work,{k:v for k,v in os.environ.items()if k in('PATH','HOME','TMPDIR')},timeout=30 if child else 60,cap=4096 if child else 65536,total_deadline=deadline)
  row.update(cwd=str(work),driverIdentity=tools[key]['before'],executionStartedOrCompleted=True);commands[-1]=row
  for name,body in streams.items():(work/(label+'.'+name)).write_bytes(body)
  save()
  assert row['driverReaped']and row['processGroupAbsentObserved']and not row['timedOut']and not row['streamExceeded'];closure();return row['exitCode'],streams['stdout'].decode().strip()
 def required(label,argv):
  code,text=command(label,argv);assert code==0;return text
 def capture():
  files={};total=0
  for p in sorted(out.rglob('*')):
   if p.is_dir():assert not p.is_symlink();continue
   if p.name=='capture-manifest.json':continue
   n=p.relative_to(out).as_posix();assert len(files)<512 and not any(x in n.split('/')for x in('.git','..'))
   b=bounded.read(p,8*1024**2);total+=len(b);assert total<=64*1024**2;files[n]=ident(b)
  context={k:os.environ[k]for k in ['GITHUB_REPOSITORY','GITHUB_REF','GITHUB_SHA','GITHUB_RUN_ID','GITHUB_RUN_ATTEMPT','GITHUB_JOB']if k in os.environ}
  (out/'capture-manifest.json').write_text(json.dumps(dict(files=files,totalBytes=total,context=context,diagnosticOnly=True,physicalAudioOrPhoneOrGameExecuted=False,SDKContentsCopied=False,sourceManifest=ident((H/'source-manifest.json').read_bytes())),sort_keys=True,indent=2)+'\n')
 oldcwd=Path.cwd();os.chdir(work)
 try:
  closure()
  for n in source:
   dst=snap/n;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(bounded.read(H/n,262144))
  (snap/'source-manifest.json').write_bytes(bounded.read(H/'source-manifest.json',65536))
  for name,(size,digest)in DEPENDENCIES.items():
   with urllib.request.urlopen('https://raw.githubusercontent.com/mackron/miniaudio/'+REV+'/'+name,timeout=30)as f:raw=f.read(size+1)
   assert len(raw)==size and hashlib.sha256(raw).hexdigest()==digest;(work/name).write_bytes(raw)
  developer=Path(required('developer',['/usr/bin/xcode-select','-p'])).resolve(strict=True);sdks={};versions={};advertised_paths={};report.update(SDKPaths=sdks,SDKVersions=versions,SDKAdvertisedPaths=advertised_paths)
  for kind in('macosx','iphoneos'):
   advertised=Path(required(kind+'-path',['/usr/bin/xcrun','--sdk',kind,'--show-sdk-path']));version=required(kind+'-version',['/usr/bin/xcrun','--sdk',kind,'--show-sdk-version']);sdk=sdk_alias(advertised,version,developer)
   advertised_paths[kind]=str(advertised);sdks[kind]=str(sdk);versions[kind]=version;save()
  clang=Path(required('clang-path',['/usr/bin/xcrun','--sdk','iphoneos','--find','clang'])).resolve(strict=True);ld=Path(required('ld-path',['/usr/bin/xcrun','--sdk','iphoneos','--find','ld'])).resolve(strict=True)
  assert clang.is_relative_to(developer)and ld.is_relative_to(developer)
  for p in(clang,ld):tools.setdefault(str(p),dict(before=tool_identity(p),after=None))
  required('clang-version',[clang,'--version']);required('xcode-version',['/usr/bin/xcodebuild','-version'])
  compiled={};links={};images={}
  def compile_unit(label,name,kind):
   target='arm64-apple-ios26.0'if kind=='iphoneos'else'arm64-apple-macos14.0';obj=work/(label+'.o');warnings=['-Werror=implicit-function-declaration']if name==PRODUCTION[0]else['-Wall','-Wextra','-Werror']
   argv=[clang,'-target',target,'-isysroot',sdks[kind],'-std=c11','-O0','-g0','-UNDEBUG','-fno-inline','-fno-lto','-fno-optimize-sibling-calls','-fstack-usage','-MMD','-MF',obj.with_suffix('.d'),'-ferror-limit=20',*warnings,'-I',H,'-I',work,'-c',H/name,'-o',obj]
   code,_=command(label,argv);compiled[label]=code
   if code==0:
    for p in(obj,obj.with_suffix('.su'),obj.with_suffix('.d')):bounded.read(p,8*1024**2)
   return obj
  mac=[compile_unit('mac-'+str(i),n,'macosx')for i,n in enumerate(UNITS)];exe=work/'own-mp3-nonzero'
  groups=[]
  for suffix in('primary','repeat'):groups.append([compile_unit('ios-'+str(i)+'-'+suffix,n,'iphoneos')for i,n in enumerate(UNITS)])
  if all(compiled['mac-'+str(i)]==0 for i in range(3)):links['mac'],_=command('mac-link',[clang,'-target','arm64-apple-macos14.0','-isysroot',sdks['macosx'],'-Wl,-fatal_warnings',*mac,'-o',exe])
  for suffix,objects in zip(('primary','repeat'),groups):
   if all(compiled['ios-'+str(i)+'-'+suffix]==0 for i in range(2)):
    image=work/('lib-memory-mp3-'+suffix+'.dylib');links[suffix],_=command('ios-link-'+suffix,[clang,'-target','arm64-apple-ios26.0','-isysroot',sdks['iphoneos'],'-dynamiclib','-Wl,-fatal_warnings','-Wl,-no_uuid','-Wl,-no_adhoc_codesign','-Wl,-no_fixup_chains','-Wl,-install_name,@rpath/lib-memory-mp3-nonzero-probe-v1.dylib',*objects[:2],'-o',image]);images[suffix]=image
  report.update(compileResults=compiled,linkResults=links)
  # All selected compile/link attempts precede independent raw audits.
  for label,code in compiled.items():
   if label.startswith('ios-')and code==0:
    try:facts[label]=inspect(bounded.read(work/(label+'.o'),8*1024**2))
    except Exception as e:errors.append(dict(stage=label,error=type(e).__name__,detail=str(e)[:512]))
  for suffix,image in images.items():
   if links[suffix]==0:
    try:facts['final-'+suffix]=inspect(bounded.read(image,8*1024**2),True)
    except Exception as e:errors.append(dict(stage='final-'+suffix,error=type(e).__name__,detail=str(e)[:512]))
  repeat_ok=len(compiled)==9 and all(code==0 for label,code in compiled.items()if label.startswith('ios-'))
  if repeat_ok:
   for x,y in zip(*groups):repeat_ok=repeat_ok and bounded.read(x,8*1024**2)==bounded.read(y,8*1024**2)and bounded.read(x.with_suffix('.su'),8*1024**2)==bounded.read(y.with_suffix('.su'),8*1024**2)
  if links.get('primary')==links.get('repeat')==0:repeat_ok=repeat_ok and bounded.read(images['primary'],8*1024**2)==bounded.read(images['repeat'],8*1024**2)
  else:repeat_ok=False
  report['repeatWholeBytesAndSU']=repeat_ok
  report['iPhoneCompileLinkStructuralProof']=len(facts)==8 and not errors and repeat_ok and all(code==0 for label,code in compiled.items()if label.startswith('ios-'))and links.get('primary')==links.get('repeat')==0
  report['native']=[];report['zeroOracleRefused']=False
  if links.get('mac')==0:
   nonce=os.urandom(16).hex();code,text=command('mac-zero-oracle',[exe,'zero-oracle',nonce],True)
   report['zeroOracleExitCode']=code;report['zeroOracleRefused']=code==91 and text=='' and (work/'mac-zero-oracle.stderr').read_bytes()==b'own zero spectral oracle refused\n'
   # This is a real OWN zero-buffer negative, never a decoder substitute.
   # Refusal is checked before any encoded positive is launched.
   if report['zeroOracleRefused']:
    cases=json.loads(bounded.read(H/'cases.json',65536))
    for case in cases:
     nonce=os.urandom(16).hex();label='mac-'+case['name'];code,text=command(label,[exe,case['name'],H/case['path'],nonce],True);row=dict(case=case['name'],exitCode=code,nonce=nonce,passed=False);report['native'].append(row)
     if code==0 and commands[-1]['stderr']['fileBytes']==0:
      try:row['observed']=parse(text,case,nonce);row['passed']=True
      except Exception as e:errors.append(dict(stage=label,error=type(e).__name__,detail=str(e)[:512]))
   report['MacNonzeroCodecFixture']=report['zeroOracleRefused'] and len(report['native'])==4 and all(row['passed']for row in report['native'])
  report['passed']=report['MacNonzeroCodecFixture']and report['iPhoneCompileLinkStructuralProof']and all(code==0 for code in compiled.values())and len(links)==3 and all(code==0 for code in links.values())
 except Exception as e:errors.append(dict(stage='runner',error=type(e).__name__,detail=str(e)[:512]));report['passed']=False
 finally:
  try:
   stable=True
   for name,row in tools.items():row['after']=tool_identity(Path(name));stable=stable and row['after']==row['before']
   closure();report['toolHashesStable']=stable;report['sourceClosureStable']=True;report['toolHashBytesRead']=hash_bytes;report['passed']=report['passed']and stable
  except Exception as e:errors.append(dict(stage='final-stability',error=type(e).__name__,detail=str(e)[:512]));report['passed']=False
  try:save();capture()
  finally:os.chdir(oldcwd)
 assert report['passed'];print(json.dumps(report))
if __name__=='__main__':main()
