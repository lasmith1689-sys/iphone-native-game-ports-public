"""One genuine SDK provider plus actual Mac pthread/kernel invocation."""
import argparse,hashlib,json,os,platform,re,shlex,struct,subprocess,sys
from pathlib import Path
SOURCES=('yield_zero.h','yield_zero_darwin.c','yield_zero_windows.c','test_yield.c','run_sdk.py','README.md')
def identity(p):return {'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
def inspect(raw):
 magic,cpu,sub,kind,count,extent,flags,reserved=struct.unpack_from('<8I',raw);assert magic==0xfeedfacf and cpu==0x100000c and kind==1 and reserved==0 and 0<count<64 and 32+extent<=len(raw);at=32;versions=[];sections=[]
 for _ in range(count):
  command,size=struct.unpack_from('<2I',raw,at);assert size>=8 and size%8==0 and at+size<=32+extent
  if command==0x32:versions.append(struct.unpack_from('<3I',raw,at+8))
  if command==0x19:
   n=struct.unpack_from('<I',raw,at+64)[0];assert size==72+n*80
   for i in range(n):
    p=at+72+i*80;name=raw[p:p+16].split(b'\0')[0].decode();length=struct.unpack_from('<Q',raw,p+40)[0];offset=struct.unpack_from('<I',raw,p+48)[0];assert offset+length<=len(raw);sections.append({'name':name,'bytes':length})
  at+=size
 assert at==32+extent and len(versions)==1 and versions[0][0]==2 and versions[0][1]==26<<16 and versions[0][2]>=27<<16
 return {'platform':2,'minimumPacked':26<<16,'SDKPacked':versions[0][2],'sections':sections}
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();assert sys.platform=='darwin' and platform.machine()=='arm64';source=Path(__file__).resolve().parent;out=a.output.resolve();assert not out.exists();out.mkdir(parents=True);(out/'logs').mkdir();(out/'primary').mkdir();(out/'repeat').mkdir();closure={n:identity(source/n) for n in SOURCES};commands=[]
 (out/'source-identities.json').write_text(json.dumps(closure,sort_keys=True,indent=2)+'\n')
 def run(label,args,timeout=120):
  command=list(map(str,args));p=subprocess.run(command,cwd=out,capture_output=True,timeout=timeout,env={**os.environ,'ZERO_AR_DATE':'1','LC_ALL':'C'});(out/'logs'/(label+'.stdout')).write_bytes(p.stdout);(out/'logs'/(label+'.stderr')).write_bytes(p.stderr);commands.append({'label':label,'command':command,'exitCode':p.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n');assert p.returncode==0,(label,p.returncode,p.stderr.decode(errors='replace')[-3000:]);return p.stdout.decode()
 clang=run('clang-path',['xcrun','--find','clang']).strip();nm=run('nm-path',['xcrun','--find','nm']).strip();ar=run('ar-path',['xcrun','--find','ar']).strip();sdk=Path(run('sdk-path',['xcrun','--sdk','iphoneos','--show-sdk-path']).strip()).resolve(strict=True);mac=Path(run('mac-sdk-path',['xcrun','--sdk','macosx','--show-sdk-path']).strip()).resolve(strict=True);resource=Path(run('resource-path',[clang,'-print-resource-dir']).strip()).resolve(strict=True);version=run('sdk-version',['xcrun','--sdk','iphoneos','--show-sdk-version']).strip();assert re.fullmatch(r'27\.\d+(?:\.\d+)?',version);run('xcode-version',['xcodebuild','-version']);run('clang-version',[clang,'--version']);flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-fno-stack-protector','-fno-builtin','-I',source];deps={};metadata=None
 for cohort in ('primary','repeat'):
  obj=out/cohort/'yield_zero_darwin.c.o';dep=obj.with_suffix('.d');run('ios-'+cohort,[clang,'-target','arm64-apple-ios26.0','-isysroot',sdk,*flags,'-ffreestanding','-fstack-usage','-MD','-MF',dep,'-c',source/'yield_zero_darwin.c','-o',obj]);current=inspect(obj.read_bytes())
  if cohort=='repeat':assert obj.read_bytes()==(out/'primary'/obj.name).read_bytes() and obj.with_suffix('.su').read_bytes()==(out/'primary'/obj.with_suffix('.su').name).read_bytes()
  else:
   metadata=current;symbols=[line.split()[-2:] for line in run('symbols',[nm,'-g',obj]).splitlines()];assert symbols==[['T','_pc_yield_zero_v1'],['U','_sched_yield']]
   paths=[Path(p).resolve(strict=True) for p in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1])];assert any(p.name=='sched.h' and p.is_relative_to(sdk) for p in paths)
   for p in paths:assert any(p.is_relative_to(root) for root in (source,sdk,resource));deps[str(p)]=identity(p)
 archive=out/'native-yield-zero-ios-arm64.a';run('archive',[ar,'rcs',archive,out/'primary/yield_zero_darwin.c.o']);assert run('archive-members',[ar,'t',archive]).splitlines()==['__.SYMDEF SORTED','yield_zero_darwin.c.o']
 exe=out/'mac-native-yield';run('mac-compile',[clang,'-arch','arm64','-isysroot',mac,*flags,source/'yield_zero_darwin.c',source/'test_yield.c','-o',exe]);outcome=json.loads(run('mac-run',[exe],60));assert outcome=={'nativeYieldCalls':3072,'workerCalls':2048,'mainCalls':1024,'workers':2,'workersSynchronizedAtGate':True,'failures':0}
 assert closure=={n:identity(source/n) for n in SOURCES};(out/'dependencies.json').write_text(json.dumps(deps,indent=2,sort_keys=True)+'\n');result={'status':'native-yield-genuine-SDK-and-Mac-pthreads-passed','AppleSDKUsed':True,'SDKVersion':version,'object':identity(out/'primary/yield_zero_darwin.c.o'),'metadata':metadata,'archive':identity(archive),'nativeMacExecutable':identity(exe),'nativeMacOutcome':outcome,'repeatedObjectAndStackUsageIdentical':True,'gameCodeIncluded':False,'gameExecuted':False,'iPhoneExecuted':False,'signed':False,'installable':False};(out/'result.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print(json.dumps({k:result[k] for k in ('status','SDKVersion','nativeMacOutcome')}))
if __name__=='__main__':main()
