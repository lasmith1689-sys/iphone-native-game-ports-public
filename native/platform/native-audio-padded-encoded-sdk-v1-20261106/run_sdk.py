"""Own explicit padded encoded composition; genuine SDK/Mac, no app/device."""
from pathlib import Path
import argparse,json,os,sys,time,urllib.request
from common import BASE,VENDOR,REV,need,pin,filepin,inputpin,save,host_guard,dep_paths,artifact_index
from bounded_process import execute,read
from image_checks import device,mac_executable,header
CORE=('ngp_audio_shared_implementation_v1.c','ngp_voice_service_v1.c','ngp_voice_tokens_v1.c','ngp_positional_provider_v1.c','ngp_audio_worker_runtime_v1.c','ngp_audio_catalog_v1.c','ngp_audio_stop_terminal_v1.c','ngp_audio_stop_os_v1.c','ngp_device_mixer_render_v1.c','ngp_mp3_envelope_v1.c','ngp_mp3_zero_prefix_v1.c','ngp_mp3_zero_prefix_decode_v1.c','ngp_wave_pcm_v1.c','ngp_encoded_voice_bind_v1.c')
PRODUCTION=(*CORE,'ngp_device_mixer_v1.m','ngp_audio_worker_broker_v1.m')
TESTS=('fixture_main.c','fixture_composed.c','test_shared.c','test_voice.c','test_prefix.c','test_fault_control.c','fixture_fault.c')
MACS=(*CORE,*TESTS)
TONE_BASE=('ngp_audio_shared_implementation_v1.c','ngp_voice_service_v1.c','ngp_device_mixer_render_v1.c','ngp_mp3_envelope_v1.c','ngp_mp3_zero_prefix_v1.c','ngp_mp3_zero_prefix_decode_v1.c','ngp_wave_pcm_v1.c','ngp_encoded_voice_bind_v1.c')
FAULT_BASE=tuple({'ngp_audio_shared_implementation_v1.c':'test_shared.c','ngp_voice_service_v1.c':'test_voice.c','ngp_mp3_zero_prefix_decode_v1.c':'test_prefix.c'}.get(n,n)for n in TONE_BASE)
REQUIRED={'_ngp_encoded_voice_bind_v1','_ngp_encoded_voice_bind_padded_mp3_v1','_ngp_memory_mp3_v1_decode','_ngp_memory_mp3_v1_view','_ngp_memory_mp3_v1_release','_ngp_mp3_envelope_v1_scan','_ngp_mp3_zero_prefix_v1_scan','_ngp_mp3_zero_prefix_v1_decode','_ngp_wave_pcm_v1_parse','_ngp_audio_worker_v1_wait_stop','_ngp_audio_catalog_v1_bind','_ngp_audio_catalog_v1_resolve_voice','_ngp_device_mixer_render_v1'}
CLAIMED=None
def case_check(kind,key,row,b):
 need(row['exitCode']==(91 if kind=='zero'else 0)and row['boundedRetirementPassed'])
 if kind=='zero':need(not b['stdout']and b['stderr']==b'own STREAM zero oracle refused\n');return dict(status='zero-oracle-refused')
 need(not b['stderr']);lines=b['stdout'].splitlines();need(len(lines)==(2 if kind=='composed'else 1));v=json.loads(lines[0])
 if kind in('old','composed'):
  name=key.split(':')[0];need(v['format']==('own-ENCODED-VOICE-v1'if kind=='old'else'own-PADDED-ENCODED-CATALOG-v1')and v['case']==name and v['copyCommitted']==1 and v['helperPoisoned']==0 and v['encodedRetiredBeforePlay']is True and v['helperReportBytes']==480 and v['allocatorViolations']==0 and v['workersParked']is True and v['deviceGuestMilesIncrementalGaplessIntegrityProof']is False)
  need(v['releaseConfirmed']==(0 if key.startswith('wave')else 1)and v['mp3ReleaseCalls']==(0 if key.startswith('wave')else 1)and v['callbackRequests']==12 and v['voiceReleaseCalls']==1)
  if kind=='composed':
   q=json.loads(lines[1]);need(q['status']=='actual-catalogue-padded-copy-render-wait-passed'and type(q['mode'])is int and q['mode']==int(key.split(':')[1])and q['requests']==1 and q['queries']==2 and q['overlapControls']==6 and q['realVoiceCatalogueDecoderWait']is True and q['brokerSynthetic']is True and q['deviceGameProof']is False and q['originalBytes']==q['suffixBytes']+q['paddingBytes']and q['paddingBytes']==(7 if q['mode']else 0));return dict(voice=v,composition=q)
 else:
  need(v['status']=='actual-encoded-typed-fault-passed'and type(v['mode'])is int and v['mode']==int(key)and v['checks']>=15 and v['reportBytes']==480 and v['realCodecVoiceLinked']is True and v['testOnlyInterceptions']is True and v['deviceGameProof']is False)
  good=int(key)in(2,3);need(v['poison']==(0 if good else 1)and v['decodes']==(2 if good else 1)and v['nested']==(1 if good else 0)and v['releases']==(2 if good else 0 if int(key)==4 else 1));need(v['firstOrigin']==(0 if good else 4 if int(key)==4 else 5 if int(key)==5 else 9 if int(key)==6 else 6 if int(key)==7 else 7))
 return v
def main():
 global CLAIMED
 ap=argparse.ArgumentParser();ap.add_argument('--execute',action='store_true');ap.add_argument('--output',required=True,type=Path);a=ap.parse_args();need(a.execute);host_guard()
 source=Path(__file__).resolve().parent;out=a.output.absolute();need(not out.exists()and not out.is_symlink()and not out.is_relative_to(source)and not any(p.is_symlink()for p in out.parents));out.mkdir(parents=True);out=out.resolve();CLAIMED=out
 for n in('logs','vendor','own-source'):(out/n).mkdir()
 manifest_raw=read(source/'manifest.json',1024**2);manifest=json.loads(manifest_raw);closure=manifest['files'];need(manifest['schema']=='own-padded-encoded-source-v1'and len(closure)==57)
 def stable():
  need(filepin(source/'manifest.json')==pin(manifest_raw))
  for n,w in closure.items():need(n==Path(n).name and filepin(source/n)==w,'source changed')
 stable()
 for n in closure:(out/'own-source'/n).write_bytes(read(source/n,8*1024**2))
 (out/'own-source/manifest.json').write_bytes(manifest_raw);save(out/'build-context.json',{n:os.environ.get(n)for n in('GITHUB_REPOSITORY','GITHUB_REF','GITHUB_SHA','GITHUB_RUN_ID','GITHUB_RUN_ATTEMPT','GITHUB_JOB')})
 commands=[];failures=[];audits=[];objects={};libraries={};cases=[];tools={};before={};actualdeps={};prior=Path.cwd();os.chdir(out);deadline=time.monotonic()+1000
 env={k:v for k,v in os.environ.items()if k in('HOME','TMPDIR','TMP','TEMP','DEVELOPER_DIR')};env.update(PATH='/usr/bin:/bin:/usr/sbin:/sbin',LC_ALL='C',ZERO_AR_DATE='1',PYTHONDONTWRITEBYTECODE='1')
 def run(label,argv,timeout=60):
  row=dict(label=label,argv=list(map(str,argv)),cwd=str(Path.cwd()),state='started-pending',exitCode=None,retirementUncertain=True);need(all(c['label']!=label for c in commands));commands.append(row);save(out/'commands.json',commands)
  try:
   stable();r,b=execute(label,row['argv'],out/'logs',env,timeout=timeout,total_deadline=deadline);row.update(r);row['state']='retired'if r['driverReaped']and r['processGroupAbsentObserved']is True else'failed-or-uncertain';row['boundedRetirementPassed']=not r['firstFailure']and not r['retirementUncertain']and not r['timedOut']and not r['streamExceeded']and r['driverReaped']and r['processGroupAbsentObserved']is True
   need(row['boundedRetirementPassed'],'bounded process failed');return row,b
  except BaseException as e:row.setdefault('driverError',dict(errorClass=type(e).__name__,message=str(e)[:512]));row['state']='failed-or-uncertain';raise
  finally:save(out/'commands.json',commands)
 def attempt(label,fn):
  try:v=fn();audits.append(dict(label=label,passed=True));return v
  except Exception as e:failures.append(dict(stage=label,errorClass=type(e).__name__,message=str(e)[:1024]));audits.append(dict(label=label,passed=False));return None
 def info(label,argv):
  r,b=run(label,argv);need(r['exitCode']==0 and not b['stderr']);return b['stdout'].decode().strip()
 try:
  for n,(size,sha)in VENDOR.items():
   with urllib.request.urlopen('https://raw.githubusercontent.com/mackron/miniaudio/'+REV+'/'+n,timeout=30)as f:b=f.read(size+1)
   need(pin(b)==dict(bytes=size,sha256=sha));(out/'vendor'/n).write_bytes(b)
  developer=Path(info('developer',['/usr/bin/xcode-select','-p'])).resolve(strict=True);need(info('xcode-version',['/usr/bin/xcodebuild','-version']).startswith('Xcode 27.1\n'));env['DEVELOPER_DIR']=str(developer)
  clang=Path(info('clang',['/usr/bin/xcrun','--find','clang'])).resolve(strict=True);need('Apple clang'in info('compiler-version',[clang,'--version']));resource=Path(info('resource',[clang,'-print-resource-dir'])).resolve(strict=True);linker=Path(info('linker',['/usr/bin/xcrun','--find','ld'])).resolve(strict=True);need(all(p.is_relative_to(developer)for p in(clang,resource,linker)))
  tools={str(p):inputpin(p)for p in(clang,linker)};save(out/'tool-identities-before.json',tools);sdks={n:Path(info(n+'-sdk',['/usr/bin/xcrun','--sdk',n,'--show-sdk-path'])).resolve(strict=True)for n in('macosx','iphoneos')};need(all(p.is_relative_to(developer)for p in sdks.values()));need(info('ios-version',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-version'])=='27.1'and info('mac-version',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-version'])=='27.0')
  save(out/'sdk-context.json',dict(developer=str(developer),clang=str(clang),linker=str(linker),resource=str(resource),sdkPaths={n:str(p)for n,p in sdks.items()},iPhoneSDK='27.1',MacSDK='27.0'))
  def args(target,n):return[clang,'-target','arm64-apple-ios26.0'if target=='iphoneos'else'arm64-apple-macos14.0','-isysroot',sdks[target],*BASE,*(['-Werror=implicit-function-declaration']if n in(CORE[0],'test_shared.c')else['-Wall','-Wextra','-Werror']),*(['-fobjc-arc','-fobjc-exceptions']if n.endswith('.m')else[]),'-I',source,'-I',out/'vendor']
  for target,names in(('iphoneos',PRODUCTION),('macosx',MACS)):
   for n in names:
    def preflight(target=target,n=n):
     dep=out/(target+'-'+n+'.pre.d');row,b=run(target+'-predeps-'+n,[*args(target,n),'-E','-o',os.devnull,'-MF',dep,source/n]);need(row['exitCode']==0 and not b['stdout']and not b['stderr'])
     for lexical in dep_paths(dep):
      p=lexical.resolve(strict=True);need(any(p.is_relative_to(root)for root in(source,out/'vendor',resource,*sdks.values())));w=inputpin(p);need(str(p)not in before or before[str(p)]==w);before[str(p)]=w
    attempt('preflight/'+target+'/'+n,preflight)
  save(out/'dependency-identities-before.json',before)
  # All compiles/available links precede audits, preserving unknown diagnostics.
  for target,names in(('iphoneos',PRODUCTION),('macosx',MACS)):
   for cohort in(('primary','repeat')if target=='iphoneos'else('primary',)):
    d=out/(target+'-'+cohort);d.mkdir()
    for n in names:
     obj=d/(n+'.o');row,b=run(target+'-'+cohort+'-'+n,[*args(target,n),'-MF',obj.with_suffix('.d'),'-c',source/n,'-o',obj]);objects[target+'/'+cohort+'/'+n]=dict(path=str(obj),exitCode=row['exitCode'],target=target,cohort=cohort,source=n,fixtureOnly=n not in PRODUCTION)
  frameworks=[x for n in('AVFAudio','AudioToolbox','Foundation','UIKit')for x in('-framework',n)];install='@rpath/lib-native-padded-encoded-ios.dylib'
  for cohort in('primary','repeat'):
   inputs=[objects['iphoneos/'+cohort+'/'+n]for n in PRODUCTION]
   if all(x['exitCode']==0 for x in inputs):
    lib=out/('padded-'+cohort+'.dylib');row,b=run('ios-link-'+cohort,[clang,'-target','arm64-apple-ios26.0','-isysroot',sdks['iphoneos'],*[x['path']for x in inputs],'-dynamiclib','-lobjc',*frameworks,'-Wl,-fatal_warnings','-Wl,-no_uuid','-Wl,-no_adhoc_codesign','-Wl,-no_fixup_chains','-Wl,-install_name,'+install,'-o',lib]);libraries[cohort]=dict(path=str(lib),exitCode=row['exitCode'],inputs=[x['path']for x in inputs])
  groups=dict(old=(*TONE_BASE,'fixture_main.c'),composed=(*CORE,'fixture_composed.c'),fault=(*FAULT_BASE,'test_fault_control.c','fixture_fault.c'))
  for group,names in groups.items():
   inputs=[objects['macosx/primary/'+n]for n in names]
   if all(x['exitCode']==0 for x in inputs):
    exe=out/('encoded-'+group);row,b=run('mac-link-'+group,[clang,'-target','arm64-apple-macos14.0','-isysroot',sdks['macosx'],*[x['path']for x in inputs],'-Wl,-fatal_warnings','-o',exe]);libraries['mac/'+group]=dict(path=str(exe),exitCode=row['exitCode'],inputs=[x['path']for x in inputs])
  for key,record in objects.items():
   def audit(record=record):
    need(record['exitCode']==0);p=Path(record['path']);raw=read(p,8*1024**2);su=read(p.with_suffix('.su'),4*1024**2);frames=[]
    for line in su.decode().splitlines():where,n,kind=line.rsplit('\t',2);need(n.isdigit()and kind=='static');frames.append(dict(location=where,bytes=int(n),kind=kind))
    need(0<len(frames)<=8192);deps=[]
    for lexical in dep_paths(p.with_suffix('.d')):
     q=lexical.resolve(strict=True);w=inputpin(q);need(str(q)in before and before[str(q)]==w);actualdeps[str(q)]=w;deps.append(dict(lexical=str(lexical),canonical=str(q),identity=w))
    m=device(raw)if record['target']=='iphoneos'else header(raw,1)
    if record['cohort']=='repeat':prior=out/'iphoneos-primary'/p.name;need(raw==read(prior,8*1024**2)and su==read(prior.with_suffix('.su'),4*1024**2))
    return dict(identity=pin(raw),stackUsage=pin(su),frames=frames,dependencies=deps,metadata=m)
   record['audit']=attempt('object/'+key,audit)
  expected={'/usr/lib/libSystem.B.dylib','/usr/lib/libobjc.A.dylib',*('/System/Library/Frameworks/'+n+'.framework/'+n for n in('AVFAudio','AudioToolbox','Foundation','UIKit','CoreFoundation'))}
  for key,record in libraries.items():
   def audit(key=key,record=record):
    need(record['exitCode']==0);raw=read(Path(record['path']),16*1024**2);m=mac_executable(raw)if key.startswith('mac/')else device(raw,True,REQUIRED)
    if not key.startswith('mac/'):need(set(m['rawFacts']['dependencies'])==expected and len(m['rawFacts']['dependencies'])==len(expected))
    return dict(identity=pin(raw),metadata=m)
   record['audit']=attempt('library/'+key,audit)
  attempt('whole-library-repeat',lambda:need(read(Path(libraries['primary']['path']),16*1024**2)==read(Path(libraries['repeat']['path']),16*1024**2)))
  def selected(label,group,argv,kind,key):
   lib=libraries.get('mac/'+group)
   if not lib or not lib.get('audit'):return
   bypath={r['path']:r for r in objects.values()};need(all(bypath[p].get('audit')for p in lib['inputs']));stable()
   for n,w in {**tools,**before}.items():need(inputpin(Path(n))==w)
   need(filepin(Path(lib['path']))==lib['audit']['identity']);row,b=run(label,[lib['path'],*argv],45)
   def validate():need(filepin(Path(lib['path']))==lib['audit']['identity']);return case_check(kind,key,row,b)
   cases.append(dict(label=label,kind=kind,key=key,group=group,actual=attempt('case/'+label,validate)))
  nonce='0123456789abcdef0123456789abcdef';negative=source/'unsupported48000.mp3'
  selected('zero-oracle','old',['zero-oracle',nonce],'zero','zero')
  for name in('mono44100','stereo44100','stereo22050','wave8','wave16'):selected('old-'+name,'old',[name,source/(name+('.wav'if name.startswith('wave')else'.mp3')),negative,nonce],'old',name)
  for name,mode in[('stereo44100',0),*((n,m)for n in('mono44100','stereo44100','stereo22050')for m in(1,2))]:selected('composed-'+name+'-'+str(mode),'composed',[name,source/(name+'.mp3'),negative,nonce,str(mode)],'composed',name+':'+str(mode))
  for mode in range(8):selected('fault-'+str(mode),'fault',[str(mode),source/'stereo44100.mp3'],'fault',str(mode))
  need(len(objects)==53 and all(r.get('audit')for r in objects.values())and len(libraries)==5 and all(r.get('audit')for r in libraries.values())and len(cases)==21 and all(c['actual']is not None for c in cases)and len(commands)==126,'complete fixed53objects/5links/21cases/126commands')
 except Exception as e:failures.append(dict(stage='driver',errorClass=type(e).__name__,message=str(e)[:1024]))
 finally:
  attempt('source-final',stable)
  for n,w in {**tools,**before}.items():attempt('final-identity:'+n,lambda n=n,w=w:need(inputpin(Path(n))==w))
  save(out/'tool-identities-after.json',{n:attempt('tool-after:'+n,lambda n=n:inputpin(Path(n)))for n in tools});save(out/'dependency-identities-actual.json',actualdeps)
  for n,v in [('objects',objects),('libraries',libraries),('native-cases',cases),('audits',audits)]:save(out/(n+'.json'),v)
  result=dict(schema='own-padded-encoded-composition-SDK-Mac-v1',wholeProof=not failures,actualProductionDeviceTUs=16,objects=len(objects),links=len(libraries),cases=len(cases),commands=len(commands),oldAPIReportABIUnchanged=True,explicitPaddedAPI=True,oneSharedImplementation=True,brokerSynthetic=True,typedFaultsOnlyInMacFixture=True,realClockForComposedWait=True,iPhoneExecuted=False,gameExecuted=False,installable=False,developerSigned=False,providerOrWholeStackOrG2Admission=False)
  save(out/'result.json',result);save(out/'failure.json',dict(success=not failures,failures=failures))
  try:save(out/'artifact-index.json',artifact_index(out))
  except Exception as e:failures.append(dict(stage='artifact-index',errorClass=type(e).__name__,message=str(e)[:512]));result['wholeProof']=False;save(out/'result.json',result);save(out/'failure.json',dict(success=False,failures=failures))
  if result['wholeProof']:(out/'success.marker').write_text('OWN_PADDED_ENCODED_COMPONENT_ONLY\n')
  os.chdir(prior)
 print(json.dumps(result));return 0 if result['wholeProof']else 1
if __name__=='__main__':
 try:code=main()
 except BaseException as e:
  if isinstance(e,SystemExit):raise
  if CLAIMED is not None:save(CLAIMED/'failure.json',dict(success=False,failures=[dict(stage='prelude-or-finalization',errorClass=type(e).__name__,message=str(e)[:1024])]))
  print(json.dumps(dict(wholeProof=False,errorClass=type(e).__name__)));code=1
 raise SystemExit(code)
