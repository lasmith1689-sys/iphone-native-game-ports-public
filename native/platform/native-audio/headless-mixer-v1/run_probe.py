"""Public own synthetic mixer diagnostic; no engine/assets/Apple device use."""
from pathlib import Path
import hashlib,json,os,platform,sys,time,urllib.request
import bounded_process as bounded
from inspect_image import inspect
H=Path(__file__).resolve().parent
REV='f40cf03f80cdb7e741d43e53b7e706e8c1394bcf'
DEPENDENCIES={'miniaudio.h':(4099492,'7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6'),'LICENSE':(2597,'457f1b500e0adf6bc059edddfa78a2f62012e7c3bb43476c20e0bd23b25ba0eb')}
def ident(raw):return dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())
def main():
 assert sys.platform=='darwin'and platform.machine()=='arm64'
 import resource
 resource.setrlimit(resource.RLIMIT_CORE,(0,0))
 out=H/'probe-output';assert not out.exists();out.mkdir();work=out/'work';work.mkdir();commands=[];tools={};facts={};deadline=time.monotonic()+900;report=dict(passed=False,syntheticSerializedMixerOnly=True,gameOrAppleAudioOrPhoneExecuted=False,toolDescendantDrainOrEscapedProcessProof=False)
 source=json.loads((H/'source-manifest.json').read_bytes())['files']
 def closure():
  for n,w in source.items():assert ident((H/n).read_bytes())==w
 def save():
  for n,value in [('report.json',report),('commands.json',commands),('tools.json',tools),('metadata.json',facts)]:
   (out/n).write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
 def command(label,argv,child=False):
  assert Path.cwd()==work
  closure();p=Path(argv[0]).resolve(strict=True);key=str(p)
  if key not in tools:tools[key]=dict(before=ident(p.read_bytes()),after=None)
  planned=dict(label=label,argv=list(map(str,argv)),cwd=str(Path.cwd()),executionStartedOrCompleted=False);commands.append(planned);save()
  row,streams=bounded.execute(label,list(map(str,argv)),work,{k:v for k,v in os.environ.items()if k in('PATH','HOME','TMPDIR')},timeout=30 if child else 60,cap=4096 if child else 65536,total_deadline=deadline)
  row.update(cwd=str(Path.cwd()),driverIdentity=tools[key]['before'],executionStartedOrCompleted=True);commands[-1]=row;save()
  # Original byte counts/truncation/prefix hashes are recorded before retaining
  # only those bounded prefixes; whole oversized streams are not an artifact.
  for n,body in streams.items():(work/(label+'.'+n)).write_bytes(body)
  assert row['driverReaped']and row['processGroupAbsentObserved']and not row['timedOut']and not row['streamExceeded']and row['exitCode']==0
  closure();return streams['stdout'].decode().strip()
 oldcwd=Path.cwd();os.chdir(work)
 try:
  closure()
  for name,(size,digest)in DEPENDENCIES.items():
   with urllib.request.urlopen('https://raw.githubusercontent.com/mackron/miniaudio/'+REV+'/'+name,timeout=30)as f:raw=f.read(size+1)
   assert len(raw)==size and hashlib.sha256(raw).hexdigest()==digest;(work/name).write_bytes(raw)
  developer=Path(command('developer',['/usr/bin/xcode-select','-p'])).resolve(strict=True);sdks={};versions={}
  for kind in('macosx','iphoneos'):
   sdk=Path(command(kind+'-path',['/usr/bin/xcrun','--sdk',kind,'--show-sdk-path'])).resolve(strict=True);version=command(kind+'-version',['/usr/bin/xcrun','--sdk',kind,'--show-sdk-version']);assert sdk.is_relative_to(developer)and sdk.is_dir()
   if kind=='iphoneos':assert version=='27.1'and sdk.name=='iPhoneOS27.1.sdk'
   else:assert int(version.split('.')[0])>=14 and sdk.name=='MacOSX'+version+'.sdk'
   sdks[kind]=str(sdk);versions[kind]=version
  clang=Path(command('clang-path',['/usr/bin/xcrun','--sdk','iphoneos','--find','clang'])).resolve(strict=True);ld=Path(command('ld-path',['/usr/bin/xcrun','--sdk','iphoneos','--find','ld'])).resolve(strict=True)
  assert clang.is_relative_to(developer)and ld.is_relative_to(developer)
  for p in(clang,ld):tools.setdefault(str(p),dict(before=ident(p.read_bytes()),after=None))
  command('clang-version',[clang,'--version']);command('xcode-version',['/usr/bin/xcodebuild','-version']);report.update(SDKPaths=sdks,SDKVersions=versions)
  def compile_unit(label,name,kind):
   target='arm64-apple-ios26.0'if kind=='iphoneos'else'arm64-apple-macos14.0';obj=work/(label+'.o')
   warnings=['-Wall','-Wextra','-Werror']if name=='fixture_main.c'else['-Werror=implicit-function-declaration']
   argv=[clang,'-target',target,'-isysroot',sdks[kind],'-std=c11','-O0','-g0','-UNDEBUG','-fno-inline','-fno-lto','-fno-optimize-sibling-calls','-fstack-usage','-MMD','-MF',obj.with_suffix('.d'),'-ferror-limit=20',*warnings,'-I',H,'-I',work,'-c',H/name,'-o',obj]
   command(label,argv)
   for p in(obj,obj.with_suffix('.su'),obj.with_suffix('.d')):assert p.stat().st_size<=8*1024**2
   if kind=='iphoneos':facts[label]=inspect(obj.read_bytes())
   return obj
  mac=[compile_unit('mac-'+str(i),n,'macosx')for i,n in enumerate(('mixer_implementation.c','fixture_main.c'))];exe=work/'synthetic-mixer'
  command('mac-link',[clang,'-target','arm64-apple-macos14.0','-isysroot',sdks['macosx'],*mac,'-o',exe])
  groups=[]
  for suffix in('primary','repeat'):
   objects=[compile_unit('ios-'+str(i)+'-'+suffix,n,'iphoneos')for i,n in enumerate(('mixer_implementation.c','fixture_main.c'))];groups.append(objects)
   image=work/('lib-headless-'+suffix+'.dylib');command('ios-link-'+suffix,[clang,'-target','arm64-apple-ios26.0','-isysroot',sdks['iphoneos'],'-dynamiclib','-Wl,-no_uuid','-Wl,-no_adhoc_codesign','-Wl,-no_fixup_chains','-Wl,-install_name,@rpath/lib-headless-mixer-probe-v1.dylib',*objects,'-o',image]);facts['ios-final-'+suffix]=inspect(image.read_bytes(),True)
  for a,b in zip(*groups):assert a.read_bytes()==b.read_bytes()and a.with_suffix('.su').read_bytes()==b.with_suffix('.su').read_bytes()
  assert(work/'lib-headless-primary.dylib').read_bytes()==(work/'lib-headless-repeat.dylib').read_bytes()
  native=json.loads(command('mac-synthetic',[exe],True));assert native['voices']==77 and native['renderAllocationViolation']is False and native['serializedSyntheticOnly']is True and native['AppleAudioOrGameExecuted']is False;report['native']=native;report['passed']=True
 finally:
  try:
   stable=True
   for name,row in tools.items():row['after']=ident(Path(name).read_bytes());stable=stable and row['after']==row['before']
   closure();report['toolHashesStable']=stable;report['passed']=report['passed']and stable;save()
  finally:os.chdir(oldcwd)
 assert report['passed'];print(json.dumps(report))
if __name__=='__main__':main()
