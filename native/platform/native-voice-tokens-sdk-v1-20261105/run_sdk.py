"""Own headless voice/token integration; actual Mac run, iOS compile only."""
import argparse,hashlib,json,os,platform,shlex,struct,subprocess,sys,urllib.request
from pathlib import Path
SOURCES=('ngp_voice_tokens_v1.c','ngp_voice_tokens_v1.h','fixture_real.c','ngp_audio_shared_impl_tokens_v1.c','ngp_audio_shared_profile_v1.h','ngp_voice_service_v1.c','ngp_voice_service_v1.h','ngp_voice_gain_balance_v2.h','inspect_image.py','run_sdk.py','README.md')
PRODUCTION=('ngp_audio_shared_impl_tokens_v1.c','ngp_voice_service_v1.c','ngp_voice_tokens_v1.c')
REV='f40cf03f80cdb7e741d43e53b7e706e8c1394bcf'
VENDOR={'miniaudio.h':(4099492,'7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6'),'LICENSE':(2597,'457f1b500e0adf6bc059edddfa78a2f62012e7c3bb43476c20e0bd23b25ba0eb')}
def identity(b):return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
def need(v):
 if not v:raise ValueError('own genuine voice-token SDK proof rejected')
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();need(sys.platform=='darwin' and platform.machine()=='arm64')
 sys.dont_write_bytecode=True
 from inspect_image import inspect
 source=Path(__file__).resolve().parent;out=a.output.resolve();need(not out.exists() and not out.is_relative_to(source));out.mkdir(parents=True);(out/'logs').mkdir();vendor=out/'vendor';vendor.mkdir()
 closure={n:identity((source/n).read_bytes()) for n in SOURCES};save(out/'source-identities.json',closure)
 def stable():need(closure=={n:identity((source/n).read_bytes()) for n in SOURCES})
 for n,(count,digest) in VENDOR.items():
  with urllib.request.urlopen('https://raw.githubusercontent.com/mackron/miniaudio/'+REV+'/'+n,timeout=30) as response:b=response.read(count+1)
  need(len(b)==count and hashlib.sha256(b).hexdigest()==digest);(vendor/n).write_bytes(b)
 commands=[];dependencies={};objects={};frames={};env={k:v for k,v in os.environ.items() if k in ('HOME','TMPDIR','TMP','TEMP','DEVELOPER_DIR')};env.update(PATH='/usr/bin:/bin:/usr/sbin:/sbin',LC_ALL='C',ZERO_AR_DATE='1',PYTHONDONTWRITEBYTECODE='1')
 def run(label,argv,timeout=120):
  stable();args=list(map(str,argv));p=subprocess.run(args,cwd=out,capture_output=True,timeout=timeout,env=env);need(len(p.stdout)+len(p.stderr)<1048576);(out/'logs'/(label+'.stdout')).write_bytes(p.stdout);(out/'logs'/(label+'.stderr')).write_bytes(p.stderr);commands.append({'label':label,'command':args,'exitCode':p.returncode});save(out/'commands.json',commands);need(p.returncode==0);return p.stdout.decode('utf8','strict').strip()
 developer=Path(run('developer',['xcode-select','-p'])).resolve(strict=True);env['DEVELOPER_DIR']=str(developer)
 clang=Path(run('clang',['xcrun','--find','clang'])).resolve(strict=True);need(clang.is_relative_to(developer));need('Apple clang' in run('compiler-version',[clang,'--version']));run('xcode-version',['xcodebuild','-version']);resource=Path(run('resource',[clang,'-print-resource-dir'])).resolve(strict=True)
 sdks={target:Path(run(target+'-sdk',['xcrun','--sdk',target,'--show-sdk-path'])).resolve(strict=True) for target in ('macosx','iphoneos')}
 need(all(p.is_relative_to(developer) for p in sdks.values()) and resource.is_relative_to(developer));need(run('ios-version',['xcrun','--sdk','iphoneos','--show-sdk-version'])=='27.1')
 mac_version=run('mac-version',['xcrun','--sdk','macosx','--show-sdk-version']);need(int(mac_version.split('.')[0])>=14)
 base=['-std=c11','-O0','-g0','-UNDEBUG','-fno-ident','-fno-inline','-fno-lto','-fno-optimize-sibling-calls','-ffp-contract=off','-fno-fast-math','-fno-strict-aliasing','-fstack-usage','-MD','-I',source,'-I',vendor,'-ferror-limit=20']
 for target in ('macosx','iphoneos'):
  for cohort in (('primary','repeat') if target=='iphoneos' else ('primary',)):
   directory=out/(target+'-'+cohort);directory.mkdir()
   for n in (*PRODUCTION,'fixture_real.c'):
    obj=directory/(n+'.o');dep=obj.with_suffix('.d');warnings=['-Werror=implicit-function-declaration'] if n==PRODUCTION[0] else ['-Wall','-Wextra','-Werror'];triple='arm64-apple-ios26.0' if target=='iphoneos' else 'arm64-apple-macos14.0'
    run(target+'-'+cohort+'-'+n,[clang,'-target',triple,'-isysroot',sdks[target],*base,*warnings,'-MF',dep,'-c',source/n,'-o',obj]);raw=obj.read_bytes();su=obj.with_suffix('.su').read_bytes();need(su and b'dynamic' not in su)
    for token in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
     path=Path(token).resolve(strict=True);need(any(path.is_relative_to(root) for root in (source,vendor,sdks[target],resource)));w=identity(path.read_bytes());need(str(path) not in dependencies or dependencies[str(path)]==w);dependencies[str(path)]=w
    if target=='iphoneos':
     meta=inspect(raw);need(meta['build']=={'platform':2,'minimum':26<<16,'sdk':(27<<16)|(1<<8)})
     if cohort=='primary':objects[n]={'identity':identity(raw),'metadata':meta};frames[n]=identity(su)
     else:need(raw==(out/'iphoneos-primary'/obj.name).read_bytes() and su==(out/'iphoneos-primary'/obj.with_suffix('.su').name).read_bytes())
 d=out/'macosx-primary';exe=out/'real-native-voice-tokens';run('mac-link',[clang,'-target','arm64-apple-macos14.0','-isysroot',sdks['macosx'],*[d/(n+'.o') for n in (*PRODUCTION,'fixture_real.c')],'-Wl,-fatal_warnings','-o',exe])
 mac=exe.read_bytes();h=struct.unpack_from('<8I',mac);need(h[:4]==(0xfeedfacf,0x100000c,0,2));need(32+h[5]<=len(mac))
 # Standard Mac linker ad-hoc signing is separate from developer/iPhone signing.
 at=32;signature=False
 for _ in range(h[4]):
  cmd,size=struct.unpack_from('<2I',mac,at);need(size>=8 and size%8==0 and at+size<=32+h[5]);signature|=cmd==0x1d;at+=size
 need(at==32+h[5]);native=json.loads(run('mac-run',[exe],60));need(native['status']=='real-voice-token-integration-passed' and native['checks']>1000 and native['cohorts']==3 and native['actualVoiceLoans']==77 and native['verifiedPCMFrames']==224 and native['renderCallbacks']==2 and native['workerStackBytes']>=8388608 and native['staleAndForeignRefused'] is True and native['actualBackend'] is True and native['physicalDeviceStarted'] is False and native['iPhoneExecuted'] is False)
 d=out/'iphoneos-primary';lib=out/'lib-native-voice-tokens-ios.dylib';run('ios-link',[clang,'-target','arm64-apple-ios26.0','-isysroot',sdks['iphoneos'],*[d/(n+'.o') for n in PRODUCTION],'-dynamiclib','-Wl,-fatal_warnings','-Wl,-no_uuid','-Wl,-no_adhoc_codesign','-Wl,-no_fixup_chains','-Wl,-install_name,@rpath/lib-native-voice-tokens-ios.dylib','-o',lib]);linked=inspect(lib.read_bytes(),final=True);need(linked['dependencies']==['/usr/lib/libSystem.B.dylib'])
 required={'_ngp_voice_tokens_v1_'+n for n in ('reserve','prepare','assign','resolve','release','close')};defined={r['name'] for r in linked['externalDefinedSymbols'] if r['type']&0xe==0xe and not r['description']&0xc0};need(required<=defined)
 need(any(n.endswith('/pthread.h') and str(sdks['iphoneos'])+'/' in n for n in dependencies));need(any(n.endswith('/stdatomic.h') for n in dependencies));stable()
 for n,w in dependencies.items():need(identity(Path(n).read_bytes())==w)
 save(out/'dependencies.json',dependencies);save(out/'actual-ios-objects.json',objects);save(out/'actual-ios-image.json',linked)
 result={'status':'own-voice-token-real-Mac-and-genuine-SDK-passed','SDKVersion':'27.1','MacSDKVersion':mac_version,'actualSDKUsed':True,'nativeMacOutcome':native,'actualMixerLinked':True,'repeatedWholeObjectsAndStackUsageIdentical':True,'objects':{n:r['identity'] for n,r in objects.items()},'stackUsage':frames,'library':identity(lib.read_bytes()),'MacExecutable':identity(mac),'MacLinkerCodeSignaturePresent':signature,'ownStrongTokenDefinitions':sorted(required),'privateGameCodeIncluded':False,'physicalDeviceStarted':False,'iPhoneExecuted':False,'gameExecuted':False,'developerSigned':False,'installable':False};save(out/'result.json',result);print(json.dumps({k:result[k] for k in ('status','SDKVersion','nativeMacOutcome')}))
if __name__=='__main__':main()
