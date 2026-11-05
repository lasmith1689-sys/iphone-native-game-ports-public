"""Genuine SDK objects and actual Mac-only retained-worker support tests."""
import argparse,hashlib,json,os,platform,re,shlex,struct,subprocess,sys
from pathlib import Path
PRODUCTION=('div64.c','monotonic.c','provider_darwin.c','virtual_counter.c','host64.c','native_required.c','native_dispatch.c','native_counter_required.c')
SOURCES=(*PRODUCTION,'div64.h','monotonic.h','provider_windows.c','virtual_counter.h','cpu64.h','native_dispatch.h','native_required.h','host64.h','local_compile_declarations.h','native_counter_required.h','test_bridge.c','vectors.inc','counter_provider_fixture.c','run_sdk.py','README.md')
BRIDGE={'_pc64_counter_binding_size_v1','_pc64_counter_binding_init_v1','_pc64_counter_bind_v1','_pc64_counter_unbind_after_stop_v1','_pc64_counter_read_active_v1','_pc64_counter_frequency_active_v1','_pc64_required_counter'}
def identity(p):return {'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
def inspect_ios(raw):
 magic,cpu,sub,kind,n,extent,flags,reserved=struct.unpack_from('<8I',raw)
 assert magic==0xfeedfacf and cpu==0x100000c and kind==1 and reserved==0 and 0<n<128 and 32+extent<=len(raw)
 at=32;versions=[];sections=[]
 for _ in range(n):
  cmd,size=struct.unpack_from('<2I',raw,at);assert size>=8 and size%8==0 and at+size<=32+extent
  if cmd==0x32:versions.append(struct.unpack_from('<3I',raw,at+8))
  if cmd==0x19:
   count=struct.unpack_from('<I',raw,at+64)[0];assert size==72+80*count
   for i in range(count):
    p=at+72+80*i;name=raw[p:p+16].split(b'\0')[0].decode();length=struct.unpack_from('<Q',raw,p+40)[0];off=struct.unpack_from('<I',raw,p+48)[0];kindflags=struct.unpack_from('<I',raw,p+64)[0]
    assert kindflags&255 in (1,12,18) or off+length<=len(raw);sections.append({'name':name,'bytes':length})
  at+=size
 assert at==32+extent and len(versions)==1 and versions[0][0]==2 and versions[0][1]==26<<16 and versions[0][2]>=27<<16
 return {'platform':2,'minimumPacked':26<<16,'SDKPacked':versions[0][2],'sections':sections}
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 assert sys.platform=='darwin' and platform.machine()=='arm64'
 source=Path(__file__).resolve().parent;out=a.output.resolve();assert not out.exists();out.mkdir(parents=True);(out/'logs').mkdir();(out/'primary').mkdir();(out/'repeat').mkdir()
 closure={n:identity(source/n) for n in SOURCES};(out/'source-identities.json').write_text(json.dumps(closure,sort_keys=True,indent=2)+'\n');commands=[]
 def run(label,args,timeout=120):
  command=list(map(str,args));p=subprocess.run(command,cwd=out,capture_output=True,timeout=timeout,env={**os.environ,'ZERO_AR_DATE':'1','LC_ALL':'C'})
  (out/'logs'/(label+'.stdout')).write_bytes(p.stdout);(out/'logs'/(label+'.stderr')).write_bytes(p.stderr);commands.append({'label':label,'command':command,'exitCode':p.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  assert p.returncode==0,(label,p.returncode,p.stderr.decode(errors='replace')[-3000:]);return p.stdout.decode()
 clang=run('clang-path',['xcrun','--find','clang']).strip();nm=run('nm-path',['xcrun','--find','nm']).strip();ar=run('ar-path',['xcrun','--find','ar']).strip()
 sdk=Path(run('sdk-path',['xcrun','--sdk','iphoneos','--show-sdk-path']).strip()).resolve(strict=True);mac=Path(run('mac-sdk-path',['xcrun','--sdk','macosx','--show-sdk-path']).strip()).resolve(strict=True)
 resource=Path(run('resource-path',[clang,'-print-resource-dir']).strip()).resolve(strict=True);version=run('sdk-version',['xcrun','--sdk','iphoneos','--show-sdk-version']).strip();assert re.fullmatch(r'27\.\d+(?:\.\d+)?',version)
 run('xcode-version',['xcodebuild','-version']);run('clang-version',[clang,'--version'])
 flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-fno-stack-protector','-fvisibility=hidden','-fno-builtin','-I',source]
 objects={};definitions=set();undefined=set();dependencies={}
 for cohort in ('primary','repeat'):
  for i,name in enumerate(PRODUCTION):
   obj=out/cohort/(name+'.o');dep=obj.with_suffix('.d')
   run(f'ios-{cohort}-{i}',[clang,'-target','arm64-apple-ios26.0','-isysroot',sdk,*flags,'-ffreestanding','-fstack-usage','-MD','-MF',dep,'-c',source/name,'-o',obj])
   metadata=inspect_ios(obj.read_bytes())
   if cohort=='repeat':assert obj.read_bytes()==(out/'primary'/obj.name).read_bytes() and obj.with_suffix('.su').read_bytes()==(out/'primary'/obj.with_suffix('.su').name).read_bytes()
   else:
    objects[name]={'identity':identity(obj),'metadata':metadata,'stackUsage':identity(obj.with_suffix('.su'))};local=set()
    for line in run(f'symbols-{i}',[nm,'-g',obj]).splitlines():
     parts=line.split();assert len(parts) in (2,3);kind,symbol=parts[-2:]
     if kind=='U':undefined.add(symbol)
     else:assert kind=='T' and symbol not in definitions;definitions.add(symbol);local.add(symbol)
    if name=='native_counter_required.c':assert local==BRIDGE
    paths=[Path(x).resolve(strict=True) for x in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1])]
    assert source/name in paths
    if name=='provider_darwin.c':assert any(p.name=='mach_time.h' and p.is_relative_to(sdk) for p in paths)
    if name=='native_counter_required.c':assert any(p.name=='pthread.h' and p.is_relative_to(sdk) for p in paths)
    for path in paths:assert any(path.is_relative_to(root) for root in (source,sdk,resource));dependencies[str(path)]=identity(path)
 external=undefined-definitions
 required={'_mach_absolute_time','_mach_timebase_info','_pthread_self','_pthread_equal','__tlv_bootstrap','_abort'}
 assert required<=external<=required|{'_memcpy','_memset','_memcmp'},external
 archive=out/'native-counter-bridge-ios-arm64.a';run('archive',[ar,'rcs',archive,*[out/'primary'/(n+'.o') for n in PRODUCTION]])
 assert run('archive-members',[ar,'t',archive]).splitlines()==['__.SYMDEF SORTED',*[n+'.o' for n in PRODUCTION]]
 tests={}
 common=[source/n for n in PRODUCTION if n!='provider_darwin.c']+[source/'test_bridge.c']
 for mode in ('controlled','real-clock'):
  exe=out/('mac-bridge-'+mode);extra=['-DPC64_COUNTER_TEST_PROVIDER=1',source/'counter_provider_fixture.c'] if mode=='controlled' else [source/'provider_darwin.c']
  run('mac-compile-'+mode,[clang,'-arch','arm64','-isysroot',mac,*flags,*common,*extra,'-o',exe]);stdout=run('mac-run-'+mode,[exe],60)
  assert stdout.startswith('PASS required-counter ') and 'workerReads=2048 workersOverlapped=1 mainBindingHeld=1 wholeStateCompared=1' in stdout and 'failedFrameDiscard=1' in stdout and f'controlledProvider={int(mode=="controlled")}' in stdout
  tests[mode]={'executable':identity(exe),'stdout':stdout}
 assert closure=={n:identity(source/n) for n in SOURCES}
 (out/'dependencies.json').write_text(json.dumps(dependencies,sort_keys=True,indent=2)+'\n')
 result={'status':'retained-counter-sdk-and-native-host-tests-passed','AppleSDKUsed':True,'SDKVersion':version,'objects':objects,'definitions':sorted(definitions),'externalSymbols':sorted(external),'repeatedObjectsIdentical':True,'archive':identity(archive),'nativeMacTests':tests,'gameCodeIncluded':False,'iPhoneExecuted':False,'gameExecuted':False,'signed':False,'installable':False}
 (out/'result.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps({k:result[k] for k in ('status','SDKVersion','repeatedObjectsIdentical','iPhoneExecuted')}))
if __name__=='__main__':main()
