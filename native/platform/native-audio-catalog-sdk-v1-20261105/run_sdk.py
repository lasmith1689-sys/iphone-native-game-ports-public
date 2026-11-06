"""Own real Mac voice-join execution and genuine iPhone broker/bridge compilation."""
from pathlib import Path
import argparse,hashlib,json,os,platform,shlex,subprocess,sys,urllib.request
COMMON=('ngp_audio_shared_impl_tokens_v1.c','ngp_voice_service_v1.c','ngp_voice_tokens_v1.c','ngp_positional_provider_v1.c','ngp_audio_worker_runtime_v1.c','ngp_audio_catalog_v1.c')
DEVICE=('ngp_device_mixer_render_v1.c','ngp_device_mixer_v1.m','ngp_audio_worker_broker_v1.m')
SOURCES=(*COMMON,*DEVICE,'ngp_audio_shared_profile_v1.h','ngp_voice_service_v1.h','ngp_voice_tokens_v1.h','ngp_positional_provider_v1.h','ngp_audio_worker_runtime_v1.h','ngp_audio_catalog_v1.h','ngp_device_mixer_render_v1.h','ngp_device_mixer_v1.h','ngp_voice_gain_balance_v2.h','fixture_real.c','inspect_image.py','run_sdk.py','README.md')
REV='f40cf03f80cdb7e741d43e53b7e706e8c1394bcf'
VENDOR={'miniaudio.h':(4099492,'7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6'),'LICENSE':(2597,'457f1b500e0adf6bc059edddfa78a2f62012e7c3bb43476c20e0bd23b25ba0eb')}
def need(value):
 if not value:raise ValueError('own actual audio-worker SDK proof rejected')
def pin(raw):return dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())
def save(path,value):path.write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',required=True,type=Path);a=ap.parse_args();need(sys.platform=='darwin'and platform.machine()=='arm64')
 sys.dont_write_bytecode=True
 from inspect_image import inspect
 source=Path(__file__).resolve().parent;out=a.output.resolve();need(not out.exists()and not out.is_relative_to(source));out.mkdir(parents=True);(out/'logs').mkdir();vendor=out/'vendor';vendor.mkdir()
 closure={n:pin((source/n).read_bytes())for n in SOURCES};need(len(closure)==22);save(out/'source-identities.json',closure)
 def stable():need(closure=={n:pin((source/n).read_bytes())for n in SOURCES})
 for name,(size,digest)in VENDOR.items():
  with urllib.request.urlopen('https://raw.githubusercontent.com/mackron/miniaudio/'+REV+'/'+name,timeout=30)as r:raw=r.read(size+1)
  need(len(raw)==size and hashlib.sha256(raw).hexdigest()==digest);(vendor/name).write_bytes(raw)
 env={k:v for k,v in os.environ.items()if k in('HOME','TMPDIR','TMP','TEMP','DEVELOPER_DIR')};env.update(PATH='/usr/bin:/bin:/usr/sbin:/sbin',LC_ALL='C',ZERO_AR_DATE='1',PYTHONDONTWRITEBYTECODE='1')
 commands=[];deps={};objects={};frames={}
 def run(label,args,timeout=120):
  stable();args=list(map(str,args));p=subprocess.run(args,cwd=out,env=env,capture_output=True,timeout=timeout);need(len(p.stdout)+len(p.stderr)<1048576)
  (out/'logs'/(label+'.stdout')).write_bytes(p.stdout);(out/'logs'/(label+'.stderr')).write_bytes(p.stderr);commands.append(dict(label=label,command=args,exitCode=p.returncode));save(out/'commands.json',commands);need(p.returncode==0);return p.stdout.decode('utf-8').strip()
 developer=Path(run('developer',['xcode-select','-p'])).resolve(strict=True);env['DEVELOPER_DIR']=str(developer)
 clang=Path(run('clang',['xcrun','--find','clang'])).resolve(strict=True);need(clang.is_relative_to(developer));need('Apple clang'in run('compiler-version',[clang,'--version']));run('xcode-version',['xcodebuild','-version'])
 resource=Path(run('resource',[clang,'-print-resource-dir'])).resolve(strict=True);need(resource.is_relative_to(developer))
 sdks={name:Path(run(name+'-sdk',['xcrun','--sdk',name,'--show-sdk-path'])).resolve(strict=True)for name in('macosx','iphoneos')};need(all(p.is_relative_to(developer)for p in sdks.values()))
 need(run('ios-version',['xcrun','--sdk','iphoneos','--show-sdk-version'])=='27.1');mac_version=run('mac-version',['xcrun','--sdk','macosx','--show-sdk-version']);need(int(mac_version.split('.')[0])>=14)
 base=['-std=c11','-O0','-g0','-UNDEBUG','-fno-ident','-fno-inline','-fno-lto','-fno-optimize-sibling-calls','-ffp-contract=off','-fno-fast-math','-fno-strict-aliasing','-fstack-usage','-MD','-I',source,'-I',vendor,'-ferror-limit=20']
 for target in('macosx','iphoneos'):
  names=(*COMMON,'fixture_real.c')if target=='macosx'else(*COMMON,*DEVICE,'fixture_real.c')
  for cohort in(('primary','repeat')if target=='iphoneos'else('primary',)):
   directory=out/(target+'-'+cohort);directory.mkdir()
   for name in names:
    obj=directory/(name+'.o');dep=obj.with_suffix('.d');triple='arm64-apple-ios26.0'if target=='iphoneos'else'arm64-apple-macos14.0'
    warnings=['-Werror=implicit-function-declaration']if name==COMMON[0]else['-Wall','-Wextra','-Werror']
    language=['-fobjc-arc','-fobjc-exceptions']if name.endswith('.m')else[]
    run(target+'-'+cohort+'-'+name,[clang,'-target',triple,'-isysroot',sdks[target],*base,*warnings,*language,'-MF',dep,'-c',source/name,'-o',obj])
    raw=obj.read_bytes();su=obj.with_suffix('.su').read_bytes();need(su and b'dynamic'not in su)
    for token in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
     p=Path(token).resolve(strict=True);need(any(p.is_relative_to(root)for root in(source,vendor,sdks[target],resource)));w=pin(p.read_bytes());need(str(p)not in deps or deps[str(p)]==w);deps[str(p)]=w
    if target=='iphoneos':
     meta=inspect(raw);need(meta['build']==dict(platform=2,minimum=26<<16,sdk=(27<<16)|(1<<8)))
     if cohort=='primary':objects[name]=dict(identity=pin(raw),metadata=meta);frames[name]=pin(su)
     else:need(raw==(out/'iphoneos-primary'/obj.name).read_bytes()and su==(out/'iphoneos-primary'/obj.with_suffix('.su').name).read_bytes())
 d=out/'macosx-primary';exe=out/'real-native-audio-catalog';run('mac-link',[clang,'-target','arm64-apple-macos14.0','-isysroot',sdks['macosx'],*[d/(n+'.o')for n in(*COMMON,'fixture_real.c')],'-Wl,-fatal_warnings','-o',exe])
 native=json.loads(run('mac-run',[exe],60));need(native['status']=='real-audio-catalog-join-passed'and native['checks']>3000 and native['actualVoiceLoans']==77 and native['cohorts']==3 and native['verifiedPCMFrames']==224 and native['renderCallbacks']==2 and native['workerStackBytes']>=8388608 and native['foreignRefused']is True and native['actualCatalogBound']is True and native['actualVoiceBackend']is True and native['brokerIsSynthetic']is True and native['physicalDeviceStarted']is False and native['iPhoneExecuted']is False)
 d=out/'iphoneos-primary';lib=out/'lib-native-audio-catalog-ios.dylib';frameworks=[flag for name in('AVFAudio','AudioToolbox','Foundation','UIKit')for flag in('-framework',name)]
 run('ios-link',[clang,'-target','arm64-apple-ios26.0','-isysroot',sdks['iphoneos'],*[d/(n+'.o')for n in(*COMMON,*DEVICE)],'-dynamiclib','-lobjc',*frameworks,'-Wl,-fatal_warnings','-Wl,-no_uuid','-Wl,-no_adhoc_codesign','-Wl,-no_fixup_chains','-Wl,-install_name,@rpath/lib-native-audio-catalog-ios.dylib','-o',lib])
 linked=inspect(lib.read_bytes(),final=True);expected={'/usr/lib/libSystem.B.dylib','/usr/lib/libobjc.A.dylib',*('/System/Library/Frameworks/'+n+'.framework/'+n for n in('AVFAudio','AudioToolbox','Foundation','UIKit','CoreFoundation'))};need(set(linked['dependencies'])==expected and len(linked['dependencies'])==len(expected))
 required={'_ngp_audio_worker_v1_open','_ngp_audio_worker_v1_borrow','_ngp_audio_worker_broker_query_v1','_ngp_audio_worker_broker_request_stop_v1','_ngp_device_mixer_render_v1'}
 required|={'_ngp_audio_catalog_v1_'+n for n in('bind','enumerate','open_provider','attribute','set_distance_factor','acquire_ordinary','acquire_positional','resolve_voice','release_voice','close_provider')}
 strong={r['name']for r in linked['externalDefinedSymbols']if r['type']&0xe==0xe and not r['description']&0xc0};need(required<=strong)
 need(any(n.endswith('/pthread.h')and'/iPhoneOS.sdk/'in n for n in deps));need(any('/UIKit.framework/Headers/'in n for n in deps));need(any('/AVFAudio.framework/Headers/'in n for n in deps));stable()
 for name,w in deps.items():need(pin(Path(name).read_bytes())==w)
 save(out/'dependencies.json',deps);save(out/'actual-ios-objects.json',objects);save(out/'actual-ios-image.json',linked)
 result=dict(status='own-audio-catalog-real-Mac-and-genuine-iPhone-broker-SDK-passed',SDKVersion='27.1',MacSDKVersion=mac_version,actualSDKUsed=True,actualVoiceBackendLinked=True,deviceBrokerAndBridgeCompiled=True,brokerIsSyntheticInMacFixture=True,nativeMacOutcome=native,repeatedWholeObjectsAndStackUsageIdentical=True,objects={n:r['identity']for n,r in objects.items()},stackUsage=frames,library=pin(lib.read_bytes()),MacExecutable=pin(exe.read_bytes()),requiredOwnStrongDefinitions=sorted(required),privateGameCodeIncluded=False,physicalDeviceStarted=False,iPhoneExecuted=False,gameExecuted=False,developerSigned=False,installable=False)
 save(out/'result.json',result);print(json.dumps(dict(status=result['status'],nativeMacOutcome=native,SDKVersion='27.1')))
if __name__=='__main__':main()
