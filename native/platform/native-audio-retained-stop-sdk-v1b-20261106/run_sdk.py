"""Future OWN real-Mac stop fixtures and full10-TU iPhone SDK build. No app/device."""
from pathlib import Path
import argparse,hashlib,json,os,platform,shlex,stat,sys,time,urllib.request
from bounded_process import execute,read,signature
from image_checks import device,mac_executable
COMMON=('ngp_audio_shared_impl_tokens_v1.c','ngp_voice_service_v1.c','ngp_voice_tokens_v1.c','ngp_positional_provider_v1.c','ngp_audio_worker_runtime_v1.c','ngp_audio_catalog_v1.c','ngp_audio_stop_terminal_v1.c')
DEVICE=('ngp_device_mixer_render_v1.c','ngp_device_mixer_v1.m','ngp_audio_worker_broker_v1.m')
PRODUCTION=(*COMMON,*DEVICE);FIXTURE='fixture_real.c'
BASE=('-std=c11','-O0','-g0','-UNDEBUG','-fno-ident','-fno-inline','-fno-lto','-fno-optimize-sibling-calls','-ffp-contract=off','-fno-fast-math','-fno-strict-aliasing','-fstack-usage','-MD','-ferror-limit=20')
REV='f40cf03f80cdb7e741d43e53b7e706e8c1394bcf'
VENDOR={'miniaudio.h':(4099492,'7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6'),'LICENSE':(2597,'457f1b500e0adf6bc059edddfa78a2f62012e7c3bb43476c20e0bd23b25ba0eb')}
AFTER=' if(s->stop_request_status&&s->stop_request_status!=NGP_DEVICE_BROKER_PENDING_V1)first_failure(s,NGP_AUDIO_WORKER_BROKER,s->stop_request_status);'
BEFORE=' if(s->stop_request_status)first_failure(s,NGP_AUDIO_WORKER_BROKER,s->stop_request_status);'
CAS=' (void)atomic_compare_exchange_strong_explicit(&s->outcome,&pending,value,memory_order_seq_cst,memory_order_seq_cst);'
STORE=' (void)pending;atomic_store_explicit(&s->outcome,value,memory_order_seq_cst);'
REQUIRED={'_ngp_audio_worker_v1_'+n for n in('open','borrow','request_stop','poll_stop')}|{'_ngp_audio_worker_broker_'+n+'_v1'for n in('query','request_stop','query_stop')}|{'_ngp_audio_catalog_v1_'+n for n in('bind','enumerate','open_provider','attribute','set_distance_factor','acquire_ordinary','acquire_positional','resolve_voice','release_voice','close_provider')}|{'_ngp_device_mixer_render_v1','_ngp_audio_stop_terminal_v1_publish','_ngp_audio_stop_terminal_v1_read'}
CLAIMED_OUTPUT=None
def need(v,message='own retained-stop build gate'):
 if not v:raise ValueError(message)
def pin(raw):return dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())
def filepin(p):return pin(read(p,16*1024**2))
def inputpin(p):
 s=p.lstat();need(stat.S_ISREG(s.st_mode)and not p.is_symlink()and s.st_size<=512*1024**2,'bounded genuine selected tool/header');h=hashlib.sha256();n=0
 with p.open('rb')as f:
  need(signature(os.fstat(f.fileno()))==signature(s))
  while True:
   b=f.read(1024**2)
   if not b:break
   n+=len(b);need(n<=s.st_size);h.update(b)
  need(signature(os.fstat(f.fileno()))==signature(s))
 need(n==s.st_size and signature(p.lstat())==signature(s));return dict(bytes=n,sha256=h.hexdigest())
def save(p,value):p.write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
def host_guard():need(sys.platform=='darwin'and platform.machine()=='arm64','genuine arm64 Darwin host required before output/network/tools')
def dep_paths(p):
 raw=read(p,4*1024**2).decode();need(':'in raw);tokens=shlex.split(raw.replace('\\\n',' ').split(':',1)[1]);need(0<len(tokens)<=8192);return[Path(n)for n in tokens]
def artifact_index(out):
 paths=sorted(out.rglob('*'));need(len(paths)<=1021,'artifact path count with manifest/marker reserve');files={};total=0
 for p in paths:
  need(not p.is_symlink(),'artifact symlink');need(p.resolve().is_relative_to(out),'artifact containment')
  if p.is_file():need(p.lstat().st_nlink==1,'single-link artifact');w=filepin(p);total+=w['bytes'];need(total<=63*1024**2,'artifact aggregate budget with metadata reserve');files[p.relative_to(out).as_posix()]=w
  else:need(p.is_dir(),'regular output tree')
 return dict(files=files,totalBytes=total,maximumBytes=64*1024**2,maximumPaths=1024)
def case_check(body,mode):
 need(type(body)is dict and body.get('status')=='real-retained-stop-case-passed'and body.get('mode')==mode and type(body.get('checks'))is int and body['checks']>=6)
 for n in('actualVoiceBackendLinked','brokerIsSynthetic'):need(body[n]is True)
 for n in('physicalDeviceStarted','iPhoneExecuted','gameExecuted'):need(body[n]is False)
 need(body['realVoiceServiceExercised']is(mode<=13))
 if mode<=13:need(body['workerStackBytes']>=8388608 and body['requests']==1 and body['realCloseBeforeRevokeObservations']==(0 if mode in(5,13)else 1))
 else:need(body['workerStackBytes']==0 and body['requests']==0)
 need(body['raceIterations']==(32 if mode==22 else 0)and body['startingRefusals']==(1 if mode==8 else 0));return body
def positive_response(row,bodies,mode):
 need(row['exitCode']==0 and row['boundedRetirementPassed']and not bodies['stderr'],'positive case exit/retirement/empty stderr');return case_check(json.loads(bodies['stdout']),mode)
def main():
 global CLAIMED_OUTPUT
 ap=argparse.ArgumentParser();ap.add_argument('--output',required=True,type=Path);a=ap.parse_args();host_guard()
 source=Path(__file__).resolve().parent;out=a.output.absolute();need(not out.exists()and not out.is_symlink()and not out.is_relative_to(source)and not any(p.is_symlink()for p in(out.parent,*out.parents)),'fresh external output');out.mkdir(parents=True);out=out.resolve();CLAIMED_OUTPUT=out;logs=out/'logs';logs.mkdir();vendor=out/'vendor';vendor.mkdir();snapshot=out/'own-source';snapshot.mkdir();mutants=out/'mutants';mutants.mkdir()
 manifest_raw=read(source/'manifest.json',1024**2);manifest_identity=pin(manifest_raw);manifest=json.loads(manifest_raw);need(manifest['schema']=='own-public-retained-stop-source-manifest-v1');closure=manifest['files'];need(len(closure)==28 and len(COMMON)==7 and len(PRODUCTION)==10)
 def stable():
  for name,w in closure.items():need(name==Path(name).name and filepin(source/name)==w,'own source closure changed')
  need(filepin(source/'manifest.json')==manifest_identity,'own manifest changed')
 stable()
 for name in closure:(snapshot/name).write_bytes(read(source/name,8*1024**2))
 (snapshot/'manifest.json').write_bytes(manifest_raw);save(out/'source-identities.json',closure);save(out/'source-manifest-identity.json',manifest_identity)
 save(out/'build-context.json',{n:os.environ.get(n)for n in('GITHUB_REPOSITORY','GITHUB_REF','GITHUB_SHA','GITHUB_RUN_ID','GITHUB_RUN_ATTEMPT','GITHUB_JOB')})
 commands=[];failures=[];audits=[];objects={};dependencies={};cases=[];tools={};header_before={};libraries={};deadline=time.monotonic()+1000;stage='public-vendor';prior_cwd=Path.cwd();os.chdir(out)
 env={k:v for k,v in os.environ.items()if k in('HOME','TMPDIR','TMP','TEMP','DEVELOPER_DIR')};env.update(PATH='/usr/bin:/bin:/usr/sbin:/sbin',LC_ALL='C',ZERO_AR_DATE='1',PYTHONDONTWRITEBYTECODE='1')
 def run(label,args,timeout=60):
  argv=list(map(str,args));record=dict(label=label,argv=argv,cwd=str(Path.cwd()),state='started-pending',exitCode=None,retirementUncertain=True);commands.append(record);save(out/'commands.json',commands)
  try:stable();row,bodies=execute(label,argv,logs,env,timeout=timeout,cap=262144,total_deadline=deadline)
  except BaseException as e:
   record.update(state='failed-or-uncertain',firstFailure=dict(stage='execute-call',errorClass=type(e).__name__,message=str(e)[:1024]),retirementUncertain=True,boundedRetirementPassed=False);save(out/'commands.json',commands);failures.append(dict(stage=label,errorClass=type(e).__name__,message=str(e)[:1024]));raise
  record.update(row);record['state']='retired'if row['driverReaped']and row['processGroupAbsentObserved']is True else'failed-or-uncertain';row=record
  ok=not row.get('firstFailure')and not row['timedOut']and not row['streamExceeded']and row['driverReaped']and row['processGroupAbsentObserved']is True;row['boundedRetirementPassed']=ok
  if not ok:failures.append(dict(stage=label,error='bounded process retirement/stream/deadline failed'))
  save(out/'commands.json',commands);return row,bodies
 def info(label,args):
  row,b=run(label,args);need(row['exitCode']==0 and row['boundedRetirementPassed'],'tool discovery failed: '+label);return b['stdout'].decode().strip()
 def attempt(label,fn):
  try:value=fn();audits.append(dict(label=label,passed=True));return value
  except Exception as e:audits.append(dict(label=label,passed=False,errorClass=type(e).__name__,message=str(e)[:1024]));failures.append(dict(stage=label,errorClass=type(e).__name__,message=str(e)[:1024]));return None
 try:
  for name,(size,digest)in VENDOR.items():
   with urllib.request.urlopen('https://raw.githubusercontent.com/mackron/miniaudio/'+REV+'/'+name,timeout=30)as response:raw=response.read(size+1)
   need(pin(raw)==dict(bytes=size,sha256=digest),'pinned public dependency');(vendor/name).write_bytes(raw)
  stage='actual-toolchain';developer=Path(info('developer',['/usr/bin/xcode-select','-p'])).resolve(strict=True);env['DEVELOPER_DIR']=str(developer);info('xcode-version',['/usr/bin/xcodebuild','-version'])
  clang=Path(info('clang',['/usr/bin/xcrun','--find','clang'])).resolve(strict=True);need(clang.is_relative_to(developer));need('Apple clang'in info('compiler-version',[clang,'--version']));resource=Path(info('resource',[clang,'-print-resource-dir'])).resolve(strict=True);need(resource.is_relative_to(developer));linker=Path(info('linker',['/usr/bin/xcrun','--find','ld'])).resolve(strict=True);need(linker.is_relative_to(developer))
  tools={str(p):inputpin(p)for p in(clang,linker)};save(out/'tool-identities-before.json',tools)
  sdks={n:Path(info(n+'-sdk',['/usr/bin/xcrun','--sdk',n,'--show-sdk-path'])).resolve(strict=True)for n in('macosx','iphoneos')};need(all(p.is_relative_to(developer)for p in sdks.values()));ios=info('ios-version',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-version']);mac=info('mac-version',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-version']);need(ios=='27.1'and int(mac.split('.')[0])>=14)
  save(out/'sdk-context.json',dict(developer=str(developer),clang=str(clang),linker=str(linker),resource=str(resource),sdkPaths={n:str(p)for n,p in sdks.items()},iPhoneSDK=ios,MacSDK=mac))
  pending=(source/'ngp_audio_worker_runtime_v1.c').read_text();terminal=(source/'ngp_audio_stop_terminal_v1.c').read_text();need(pending.count(AFTER)==1 and terminal.count(CAS)==1)
  controls={'old-pending-worker.c':pending.replace(AFTER,BEFORE),'overwrite-terminal.c':terminal.replace(CAS,STORE)}
  for name,text in controls.items():(mutants/name).write_text(text)
  control_pins={n:filepin(mutants/n)for n in controls};save(out/'mutant-source-identities.json',{n:dict(identity=control_pins[n],controlOnly=True,excludedFromDeviceLibrary=True)for n in controls})
  def args(target,name):
   triple='arm64-apple-ios26.0'if target=='iphoneos'else'arm64-apple-macos14.0';warn=['-Werror=implicit-function-declaration']if name==COMMON[0]else['-Wall','-Wextra','-Werror'];objc=['-fobjc-arc','-fobjc-exceptions']if name.endswith('.m')else[]
   return[clang,'-target',triple,'-isysroot',sdks[target],*BASE,*warn,*objc,'-I',source,'-I',vendor]
  stage='actual-header-preflight'
  for target,names in(('iphoneos',(*PRODUCTION,FIXTURE)),('macosx',(*COMMON,FIXTURE))):
   for name in names:
    dep=out/(target+'-'+name+'.pre.d');row,bodies=run(target+'-predeps-'+name,[*args(target,name),'-E','-o',os.devnull,'-MF',dep,source/name])
    if row['exitCode']!=0:failures.append(dict(stage=row['label'],exitCode=row['exitCode']));continue
    need(not bodies['stdout'],'predependency stdout must remain empty')
    for lexical in dep_paths(dep):
     p=lexical.resolve(strict=True);need(any(p.is_relative_to(root)for root in(source,vendor,resource,*sdks.values())),'predependency escaped selected roots');w=inputpin(p);need(str(p)not in header_before or header_before[str(p)]==w);header_before[str(p)]=w
  for sdk in sdks.values():
   p=sdk/'SDKSettings.json'
   if p.exists():header_before[str(p.resolve())]=inputpin(p.resolve())
  save(out/'dependency-identities-before.json',header_before)
  # Capture ALL actual compiles and ALL available links BEFORE any parser,
  # frame, dependency or repeat audit can stop later-unit evidence collection.
  stage='all-actual-compiles'
  for target,names in(('iphoneos',(*PRODUCTION,FIXTURE)),('macosx',(*COMMON,FIXTURE))):
   for cohort in(('primary','repeat')if target=='iphoneos'else('primary',)):
    d=out/(target+'-'+cohort);d.mkdir()
    for name in names:
     obj=d/(name+'.o');row,_=run(target+'-'+cohort+'-'+name,[*args(target,name),'-MF',obj.with_suffix('.d'),'-c',source/name,'-o',obj]);objects[target+'/'+cohort+'/'+name]=dict(path=str(obj),exitCode=row['exitCode'],target=target,cohort=cohort,source=name,fixtureOnly=name==FIXTURE)
  for name in controls:
   obj=mutants/(name+'.o');row,_=run('mac-mutant-'+name,[*args('macosx',name),'-MF',obj.with_suffix('.d'),'-c',mutants/name,'-o',obj]);objects['mutants/'+name]=dict(path=str(obj),exitCode=row['exitCode'],target='macosx',cohort='mutant',source=name,fixtureOnly=True)
  stage='all-available-strict-links';frameworks=[f for n in('AVFAudio','AudioToolbox','Foundation','UIKit')for f in('-framework',n)];link_common=['-Wl,-fatal_warnings'];install='@rpath/lib-native-audio-retained-stop-ios.dylib'
  for cohort in('primary','repeat'):
   entries=[objects['iphoneos/'+cohort+'/'+n]for n in PRODUCTION]
   if all(x['exitCode']==0 for x in entries):
    lib=out/('lib-native-audio-retained-stop-ios'+('-repeat'if cohort=='repeat'else'')+'.dylib');row,_=run('ios-link-'+cohort,[clang,'-target','arm64-apple-ios26.0','-isysroot',sdks['iphoneos'],*[x['path']for x in entries],'-dynamiclib','-lobjc',*frameworks,*link_common,'-Wl,-no_uuid','-Wl,-no_adhoc_codesign','-Wl,-no_fixup_chains','-Wl,-install_name,'+install,'-o',lib]);libraries[cohort]=dict(path=str(lib),exitCode=row['exitCode'])
  mac_entries=[objects['macosx/primary/'+n]for n in(*COMMON,FIXTURE)]
  if all(x['exitCode']==0 for x in mac_entries):
   for control,replacement in((None,None),('old-pending-worker.c','ngp_audio_worker_runtime_v1.c'),('overwrite-terminal.c','ngp_audio_stop_terminal_v1.c')):
    if control and objects['mutants/'+control]['exitCode']!=0:continue
    exe=out/('real-retained-stop'+('-'+control if control else''));inputs=[objects['mutants/'+control]['path']if control and x['source']==replacement else x['path']for x in mac_entries];row,_=run('mac-link-'+(control or'primary'),[clang,'-target','arm64-apple-macos14.0','-isysroot',sdks['macosx'],*inputs,*link_common,'-o',exe]);libraries['mac/'+str(control)]=dict(path=str(exe),exitCode=row['exitCode'])
  stage='independent-object-frame-dependency-audits'
  for label,record in objects.items():
   obj=Path(record['path'])
   if record['exitCode']!=0:failures.append(dict(stage=label,exitCode=record['exitCode']));continue
   def audit_object():
    raw=read(obj,8*1024**2);su=read(obj.with_suffix('.su'),4*1024**2);need(su and len(su.splitlines())<=8192);frames=[]
    for line in su.decode().splitlines():where,n,kind=line.rsplit('\t',2);need(n.isdigit()and kind=='static');frames.append(dict(compilerLocationAndFunction=where,localCompilerFrameBytes=int(n),kind=kind))
    actual=[]
    for lexical in dep_paths(obj.with_suffix('.d')):
     p=lexical.resolve(strict=True);w=inputpin(p)
     if p.is_relative_to(mutants):need(p.name in controls and w==control_pins[p.name])
     else:need(str(p)in header_before and header_before[str(p)]==w,'new/changed actual dependency')
     dependencies[str(p)]=w;actual.append(dict(lexical=str(lexical),canonical=str(p),identity=w))
    meta=device(raw)if record['target']=='iphoneos'else dict(rawRetainedForFutureReadback=True)
    if record['cohort']=='repeat':prior=out/'iphoneos-primary'/obj.name;need(raw==read(prior,8*1024**2)and su==read(prior.with_suffix('.su'),4*1024**2),'whole object/SU repeat mismatch')
    return dict(identity=pin(raw),stackUsage=pin(su),frames=frames,dependencies=actual,metadata=meta)
   record['audit']=attempt(label,audit_object)
  stage='independent-linked-image-audits';expected={'/usr/lib/libSystem.B.dylib','/usr/lib/libobjc.A.dylib',*('/System/Library/Frameworks/'+n+'.framework/'+n for n in('AVFAudio','AudioToolbox','Foundation','UIKit','CoreFoundation'))}
  for name,record in libraries.items():
   if record['exitCode']!=0:failures.append(dict(stage='linked/'+name,exitCode=record['exitCode']));continue
   def audit_library():
    raw=read(Path(record['path']),16*1024**2)
    if name.startswith('mac/'):return dict(identity=pin(raw),metadata=mac_executable(raw))
    m=device(raw,True,REQUIRED);need(set(m['rawFacts']['dependencies'])==expected and len(m['rawFacts']['dependencies'])==len(expected));return dict(identity=pin(raw),metadata=m)
   record['audit']=attempt('linked/'+name,audit_library)
  if all(n in libraries and libraries[n].get('audit')for n in('primary','repeat')):attempt('whole-library-repeat',lambda:need(read(Path(libraries['primary']['path']),16*1024**2)==read(Path(libraries['repeat']['path']),16*1024**2),'whole library repeat mismatch'))
  stage='actual-real-Mac-fixtures';expect=json.loads(read(source/'expected_outcomes.json',1024**2))
  primary=libraries.get('mac/None')
  if primary and primary.get('audit'):
   for mode in expect['positiveModes']:
    row,b=run('mac-case-'+str(mode),[primary['path'],str(mode)],45)
    def positive():need(filepin(Path(primary['path']))==primary['audit']['identity']);return positive_response(row,b,mode)
    cases.append(dict(mode=mode,expected='positive',actual=attempt('mac-case/'+str(mode),positive)))
  for control,mode,assertion in((x['source'],x['mode'],x['assertion'])for x in expect['negativeControls']):
   record=libraries.get('mac/'+control)
   if not record or not record.get('audit'):continue
   row,b=run('mac-negative-'+control,[record['path'],str(mode)],45)
   def negative():
    need(row['exitCode']==101 and row['boundedRetirementPassed']and not b['stdout']);r=json.loads(b['stderr']);need(r['status']=='fixture-assertion-failed'and r['id']==assertion and type(r['line'])is int);need(filepin(Path(record['path']))==record['audit']['identity']);return r
   cases.append(dict(mode=mode,control=control,expected='assertion-exit101',actual=attempt('negative/'+control,negative)))
  need(len(cases)==20 and all(c['actual']is not None for c in cases),'all18positive+2negative Mac outcomes required');need(len(objects)==32 and all(x.get('audit')for x in objects.values()),'all32 real compile outputs required');need(len(libraries)==5 and all(x.get('audit')for x in libraries.values()),'all5 actual links required');need(len(commands)==86,'exact command retirement roster')
 except Exception as e:failures.append(dict(stage=stage,errorClass=type(e).__name__,message=str(e)[:1024]))
 finally:
  stage='final-stability-and-retention';attempt('source-final-stability',stable)
  for name,w in {**tools,**header_before}.items():attempt('final-identity:'+name,lambda n=name,x=w:need(inputpin(Path(n))==x,'selected tool/header changed'))
  for name,w in locals().get('control_pins',{}).items():attempt('final-mutant:'+name,lambda n=name,x=w:need(filepin(mutants/n)==x,'selected mutant changed'))
  tool_after={n:attempt('tool-after:'+n,lambda p=n:inputpin(Path(p)))for n in tools}
  save(out/'dependency-identities-actual.json',dependencies);save(out/'objects.json',objects);save(out/'libraries.json',libraries);save(out/'native-cases.json',cases);save(out/'audits.json',audits);save(out/'tool-identities-after.json',tool_after)
  failure=dict(failures=failures,success=False if failures else True);save(out/'failure.json',failure)
  result=dict(schema='own-retained-stop-full-cohort-SDK-Mac-result-v1',wholeProof=not failures,actualProductionDeviceTUs=10,optionalFixtureDeviceTUs=1,actualCompileOutputCount=len(objects),actualLinkCount=len(libraries),actualMacCaseCount=len(cases),actualSDKUsed='iphoneos' in locals().get('sdks',{}),expectedSDK='27.1',fullRepeatedSourceAndFrameReadbackRequested=True,oldObjectsOrFramesReused=False,fixtureExcludedFromDeviceLibrary=True,MacBrokerIsExplicitlySynthetic=True,physicalDeviceStarted=False,iPhoneExecuted=False,gameExecuted=False,developerSigned=False,installable=False,providerOrTotalStackOrG2Admission=False)
  save(out/'result.json',result)
  try:save(out/'artifact-index.json',artifact_index(out))
  except Exception as e:failures.append(dict(stage='artifact-index',errorClass=type(e).__name__,message=str(e)[:1024]));result['wholeProof']=False;save(out/'failure.json',dict(failures=failures,success=False));save(out/'result.json',result)
  if result['wholeProof']:(out/'success.marker').write_text('OWN_COMPONENT_PROOF_ONLY\n')
  os.chdir(prior_cwd)
 print(json.dumps(dict(wholeProof=result['wholeProof'],actualProductionDeviceTUs=10,actualCompileOutputCount=len(objects),actualLinkCount=len(libraries),actualMacCaseCount=len(cases),physicalDeviceStarted=False)))
 return 0 if result['wholeProof']else 1
if __name__=='__main__':
 try:code=main()
 except BaseException as e:
  if isinstance(e,SystemExit):raise
  if CLAIMED_OUTPUT is not None:
   save(CLAIMED_OUTPUT/'failure.json',dict(success=False,failures=[dict(stage='owned-prelude-or-finalization',errorClass=type(e).__name__,message=str(e)[:1024])]))
   save(CLAIMED_OUTPUT/'result.json',dict(wholeProof=False,physicalDeviceStarted=False,iPhoneExecuted=False,gameExecuted=False,installable=False))
  print(json.dumps(dict(wholeProof=False,errorClass=type(e).__name__,physicalDeviceStarted=False)));code=1
 raise SystemExit(code)
