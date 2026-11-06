"""Future actual-C parent counterexamples; import/offline checks execute nothing."""
from pathlib import Path
import argparse,hashlib,json,os,platform,shlex,stat,sys,time,urllib.request
from bounded_process import execute,read,signature
from image_checks import header,mac_executable
COMMON=('ngp_audio_shared_impl_tokens_v1.c','ngp_voice_service_v1.c','ngp_voice_tokens_v1.c','ngp_positional_provider_v1.c','ngp_audio_worker_runtime_v1.c','ngp_audio_catalog_v1.c','ngp_audio_stop_terminal_v1.c')
FIXTURE='fixture_parent.c'
BASE=('-std=c11','-O0','-g0','-UNDEBUG','-fno-ident','-fno-inline','-fno-lto','-fno-optimize-sibling-calls','-ffp-contract=off','-fno-fast-math','-fno-strict-aliasing','-fstack-usage','-MD','-ferror-limit=20')
VENDOR={'miniaudio.h':(4099492,'7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6'),'LICENSE':(2597,'457f1b500e0adf6bc059edddfa78a2f62012e7c3bb43476c20e0bd23b25ba0eb')}
REV='f40cf03f80cdb7e741d43e53b7e706e8c1394bcf'
def need(v,message='parent contract evidence gate'):
 if not v:raise ValueError(message)
def pin(b):return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def save(p,v):p.write_text(json.dumps(v,sort_keys=True,indent=2)+'\n')
def host_guard():need(sys.platform=='darwin'and platform.machine()=='arm64','genuine arm64 Darwin required before output/network/tools')
def inputpin(p):
 s=p.lstat();need(stat.S_ISREG(s.st_mode)and not p.is_symlink()and s.st_size<=512*1024**2);h=hashlib.sha256();n=0
 with p.open('rb')as f:
  need(signature(os.fstat(f.fileno()))==signature(s))
  while True:
   b=f.read(1024**2)
   if not b:break
   n+=len(b);need(n<=s.st_size);h.update(b)
  need(signature(os.fstat(f.fileno()))==signature(s))
 need(n==s.st_size and signature(p.lstat())==signature(s));return dict(bytes=n,sha256=h.hexdigest())
def dep_paths(p):
 body=read(p,4*1024**2).decode();need(':'in body);tokens=shlex.split(body.replace('\\\n',' ').split(':',1)[1]);need(0<len(tokens)<=8192);return[Path(t)for t in tokens]
def case_check(mode,row,bodies):
 need(mode in(0,1,2)and row['boundedRetirementPassed']is True,'case process did not retire')
 lines=bodies['stdout'].decode().splitlines();need(len(lines)==(2 if mode==0 else 1),'unexpected observation count');observed=json.loads(lines[0])
 need(set(observed)=={'status','mode','code','phase','firstWorkerStatus','closeAck','requests','queries','realCloseObserved','logicalNow','deadline','externalTimeoutRecorded','checks','workerStackBytes','brokerIsSynthetic','externalClockIsSynthetic','actualVoiceRuntimeLinked','physicalDevice','game'})
 need(observed['status']=='parent-contract-observation'and type(observed['mode'])is int and observed['mode']==mode)
 for n in('code','phase','firstWorkerStatus','closeAck','requests','queries','realCloseObserved','logicalNow','deadline','externalTimeoutRecorded','checks','workerStackBytes'):need(type(observed[n])is int)
 need(observed['code']==0 and observed['phase']==5 and observed['firstWorkerStatus']==0 and observed['closeAck']==1 and observed['requests']==1 and observed['realCloseObserved']==1)
 need(observed['queries']==(1 if mode==1 else 2)and observed['workerStackBytes']>=8388608 and 20<=observed['checks']<=256)
 need(observed['brokerIsSynthetic']is True and observed['externalClockIsSynthetic']is True and observed['actualVoiceRuntimeLinked']is True and observed['physicalDevice']is False and observed['game']is False)
 need(observed['deadline']==5000000100 and observed['logicalNow']==(100 if mode==0 else 5000000100)and observed['externalTimeoutRecorded']==(1 if mode==2 else 0))
 if mode==0:
  need(row['exitCode']==0 and not bodies['stderr']);final=json.loads(lines[1]);need(set(final)=={'status','mode','checks'}and final['status']=='parent-baseline-control-passed'and type(final['mode'])is int and final['mode']==0 and type(final['checks'])is int and observed['checks']<=final['checks']<=256)
 else:
  need(row['exitCode']==101);err=json.loads(bodies['stderr']);need(set(err)=={'status','id','line'}and err['status']=='fixture-assertion-failed'and type(err['line'])is int and err['line']>0)
  need(err['id']==('late-ack-must-not-publish-stopped'if mode==1 else'external-timeout-must-stay-worker-sticky'))
 return observed
def main():
 a=argparse.ArgumentParser();a.add_argument('--execute',action='store_true');a.add_argument('--output',type=Path,required=True);args=a.parse_args();need(args.execute,'explicit future execution required');host_guard()
 source=Path(__file__).resolve().parent;out=args.output.absolute();need(not out.exists()and not out.is_symlink()and not out.is_relative_to(source)and not any(p.is_symlink()for p in out.parents));out.mkdir(parents=True);out=out.resolve();logs=out/'logs';logs.mkdir();vendor=out/'vendor';vendor.mkdir();objects_dir=out/'objects';objects_dir.mkdir();snapshot=out/'own-source';snapshot.mkdir()
 raw_manifest=read(source/'manifest.json',1024**2);manifest=json.loads(raw_manifest);need(manifest['schema']=='own-parent-contract-fixture-manifest-v1');closure=manifest['files'];need(len(closure)==28 and all(n==Path(n).name for n in closure))
 def stable():
  need(pin(read(source/'manifest.json',1024**2))==pin(raw_manifest))
  for n,w in closure.items():need(pin(read(source/n,8*1024**2))==w,'source changed')
 stable()
 for n in closure:(snapshot/n).write_bytes(read(source/n,8*1024**2))
 (snapshot/'manifest.json').write_bytes(raw_manifest)
 save(out/'build-context.json',{n:os.environ.get(n)for n in('GITHUB_REPOSITORY','GITHUB_REF','GITHUB_RUN_ID','GITHUB_RUN_ATTEMPT','GITHUB_SHA','GITHUB_JOB')})
 commands=[];failures=[];objects={};cases=[];audits=[];before={};tools={};deadline=time.monotonic()+600;previous=Path.cwd();os.chdir(out)
 def attempt(label,fn):
  try:value=fn();audits.append(dict(label=label,passed=True));return value
  except BaseException as e:failures.append(dict(stage=label,errorClass=type(e).__name__,message=str(e)[:512]));audits.append(dict(label=label,passed=False));return None
 def run(label,argv):
  need(all(c['label']!=label for c in commands));row=dict(label=label,argv=[str(x)for x in argv],cwd=str(Path.cwd()),state='started-pending');commands.append(row);save(out/'commands.json',commands)
  try:
   result,bodies=execute(label,row['argv'],logs,dict(os.environ),timeout=45 if label.startswith('case-')else 60,total_deadline=deadline);row.update(result);row['state']='retired';row['boundedRetirementPassed']=not result['retirementUncertain']and not result['firstFailure']and not result['timedOut']and not result['streamExceeded'];need(row['boundedRetirementPassed'],'bounded process failed');return row,bodies
  except BaseException as e:row.update(state='failed-or-uncertain',driverError=dict(errorClass=type(e).__name__,message=str(e)[:512]));row.setdefault('retirementUncertain',True);raise
  finally:save(out/'commands.json',commands)
 def info(label,argv):
  row,b=run(label,argv);need(row['exitCode']==0 and not b['stderr']);return b['stdout'].decode().strip()
 try:
  for n,(size,sha)in VENDOR.items():
   with urllib.request.urlopen('https://raw.githubusercontent.com/mackron/miniaudio/'+REV+'/'+n,timeout=30)as response:b=response.read(size+1)
   need(pin(b)==dict(bytes=size,sha256=sha));(vendor/n).write_bytes(b)
  developer=Path(info('developer',['/usr/bin/xcode-select','-p'])).resolve(strict=True);need(info('xcode-version',['/usr/bin/xcodebuild','-version']).startswith('Xcode 27.1\n'))
  clang=Path(info('clang',['/usr/bin/xcrun','--find','clang'])).resolve(strict=True);need(clang.is_relative_to(developer));need('Apple clang'in info('compiler-version',[clang,'--version']))
  resource=Path(info('resource',[clang,'-print-resource-dir'])).resolve(strict=True);linker=Path(info('linker',['/usr/bin/xcrun','--find','ld'])).resolve(strict=True);sdk=Path(info('sdk',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'])).resolve(strict=True);sdk_version=info('sdk-version',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-version']);need(resource.is_relative_to(developer)and linker.is_relative_to(developer)and sdk.is_relative_to(developer)and sdk_version=='27.0')
  tools={str(p):inputpin(p)for p in(clang,linker)};save(out/'tool-identities-before.json',tools);save(out/'sdk-context.json',dict(developer=str(developer),clang=str(clang),linker=str(linker),resource=str(resource),sdk=str(sdk),sdkVersion=sdk_version,target='arm64-apple-macos14.0'))
  def argv(n):return[clang,'-target','arm64-apple-macos14.0','-isysroot',sdk,*BASE,*(['-Werror=implicit-function-declaration']if n==COMMON[0]else['-Wall','-Wextra','-Werror']),'-I',source,'-I',vendor]
  for n in(*COMMON,FIXTURE):
   def preflight(n=n):
    dep=out/(n+'.pre.d');row,b=run('predeps-'+n,[*argv(n),'-E','-o',os.devnull,'-MF',dep,source/n]);need(row['exitCode']==0 and not b['stdout']and not b['stderr'])
    for lexical in dep_paths(dep):
     p=lexical.resolve(strict=True);need(any(p.is_relative_to(root)for root in(source,vendor,resource,sdk)));w=inputpin(p);need(str(p)not in before or before[str(p)]==w);before[str(p)]=w
   attempt('predeps-'+n,preflight)
  save(out/'dependency-identities-before.json',before)
  for n in(*COMMON,FIXTURE):
   obj=objects_dir/(n+'.o');row,b=run('compile-'+n,[*argv(n),'-MF',obj.with_suffix('.d'),'-c',source/n,'-o',obj]);objects[n]=dict(path=str(obj),exitCode=row['exitCode'])
  executable=out/'parent-contract-fixture';linked=False
  if all(x['exitCode']==0 for x in objects.values()):
   row,b=run('link',[clang,'-target','arm64-apple-macos14.0','-isysroot',sdk,*[objects[n]['path']for n in(*COMMON,FIXTURE)],'-Wl,-fatal_warnings','-o',executable]);linked=row['exitCode']==0
  for n,r in objects.items():
   def object_audit(n=n,r=r):
    need(r['exitCode']==0);obj=Path(r['path']);raw=read(obj,8*1024**2);m=header(raw,1);need(m['build']==dict(platform=1,minimum=14<<16,sdk=27<<16));su=read(obj.with_suffix('.su'),4*1024**2);frames=[]
    for line in su.decode().splitlines():where,size,kind=line.rsplit('\t',2);need(size.isdigit()and kind=='static');frames.append(dict(location=where,bytes=int(size),kind=kind))
    need(frames and len(frames)<=8192);deps=[]
    for lexical in dep_paths(obj.with_suffix('.d')):
     p=lexical.resolve(strict=True);w=inputpin(p);need(str(p)in before and before[str(p)]==w);deps.append(dict(lexical=str(lexical),canonical=str(p),identity=w))
    return dict(identity=pin(raw),stackUsage=pin(su),frames=frames,dependencies=deps,rawBuild=m['build'],completeStackAdmission=False)
   r['audit']=attempt('object-'+n,object_audit)
  image=attempt('linked-image',lambda:dict(identity=pin(read(executable,16*1024**2)),metadata=mac_executable(read(executable,16*1024**2))))if linked else None
  attempt('source-before-native',stable)
  for n,w in {**tools,**before}.items():attempt('before-native-identity:'+n,lambda n=n,w=w:need(inputpin(Path(n))==w))
  if image is not None and not failures:
   for mode in(0,1,2):
    def native_case(mode=mode):
     row,b=run('case-'+str(mode),[executable,str(mode)]);observed=case_check(mode,row,b);cases.append(dict(mode=mode,observed=observed,expectedAssertionFailure=mode!=0));return observed
    attempt('case-'+str(mode),native_case)
  attempt('source-final',stable)
  for n,w in {**tools,**before}.items():attempt('final-identity:'+n,lambda n=n,w=w:need(inputpin(Path(n))==w))
  save(out/'tool-identities-after.json',{n:attempt('tool-after:'+n,lambda n=n:inputpin(Path(n)))for n in tools})
  need(len(commands)==28,'fixed8tool/8predeps/8compile/1link/3case count');need(len(cases)==3,'all parent outcomes required')
  result=dict(schema='actual-parent-missing-wait-contract-capture',parentCounterexamplesObserved=not failures,baselineControlPassed=any(x['mode']==0 for x in cases),actualCCounterexampleCount=sum(x['expectedAssertionFailure']for x in cases),productionWaitImplemented=False,newWaitContractPassed=False,iPhoneSDKUsed=False,actualMacSDKUsed=True,brokerIsSynthetic=True,externalClockIsSynthetic=True,physicalDevice=False,game=False)
  save(out/'result.json',result)
 except BaseException as e:failures.append(dict(stage='driver',errorClass=type(e).__name__,message=str(e)[:512]))
 finally:
  save(out/'objects.json',objects);save(out/'native-cases.json',cases);save(out/'audits.json',audits);save(out/'failure.json',dict(success=not failures,failures=failures));os.chdir(previous)
 if not failures:(out/'success.marker').write_text('PARENT_COUNTEREXAMPLES_ONLY\n')
 raise SystemExit(0 if not failures else 1)
if __name__=='__main__':main()
