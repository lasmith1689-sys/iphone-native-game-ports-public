"""Finite real Mac pthread tests and genuine iPhone compile; never game/device."""
import argparse,hashlib,json,os,platform,re,shlex,struct,subprocess,sys,time,traceback
from pathlib import Path
sys.dont_write_bytecode=True
SOURCES=('ngp_fls_v1.c','ngp_fls_v1.h','ngp_fls_protocol_v1.c','ngp_fls_protocol_v1.h','ngp_worker_tag_v1.c','ngp_worker_tag_v1.h','ngp_worker_tag_policy_v1.h','ngp_fls_environment_v1.h','synthetic_environment.c','synthetic_environment.h','fixture_pthread.c','schedule.json','run_apple.py','inspect_sdk.py','README.md')
PRODUCTION=('ngp_fls_v1.c','ngp_fls_protocol_v1.c','ngp_worker_tag_v1.c')
def valid_label(label):return isinstance(label,str)and len(label)<=96 and re.fullmatch('[a-z0-9_-]+',label)is not None
def pin(p):
 b=p.read_bytes();return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args();source=Path(__file__).resolve().parent;out=args.output.resolve()
 assert out.parent==source.parent and out.name=='output'and not out.exists()and not source.is_symlink();out.mkdir();(out/'logs').mkdir();commands=[];frames={};metadata={};closure={};completed=[];started=time.monotonic();gate='source';retirementUncertain=False;status='failed';failedType=None;failureDetail=None
 def stable():assert set(p.name for p in source.iterdir())==set(SOURCES)and all(not(source/n).is_symlink()and pin(source/n)==closure[n]for n in SOURCES)
 def run(label,argv,timeout=60):
  nonlocal gate,retirementUncertain
  assert valid_label(label);gate=label;remaining=540-(time.monotonic()-started);assert remaining>0;begin=time.monotonic();limit=min(timeout,remaining)
  try:p=subprocess.run(list(map(str,argv)),cwd=out,capture_output=True,timeout=limit,env={**os.environ,'ZERO_AR_DATE':'1','LC_ALL':'C'})
  except BaseException:retirementUncertain=True;raise
  assert len(p.stdout)<=262144 and len(p.stderr)<=262144;(out/'logs'/f'{label}.stdout').write_bytes(p.stdout);(out/'logs'/f'{label}.stderr').write_bytes(p.stderr);commands.append(dict(label=label,argv=list(map(str,argv)),exitCode=p.returncode,elapsedSeconds=time.monotonic()-begin,timeout=False,stdout=dict(bytes=len(p.stdout),sha256=hashlib.sha256(p.stdout).hexdigest()),stderr=dict(bytes=len(p.stderr),sha256=hashlib.sha256(p.stderr).hexdigest())));return p
 def ok(label,argv):
  p=run(label,argv);assert p.returncode==0;return p.stdout.decode().strip()
 try:
  closure={n:pin(source/n)for n in SOURCES};stable();gate='host';assert sys.platform=='darwin'and platform.machine()=='arm64'
  from inspect_sdk import parse,destructor,REQUIRED,ENV,SYSTEM
  clang=ok('clang-path',['/usr/bin/xcrun','--find','clang']);ar=ok('ar-path',['/usr/bin/xcrun','--find','ar']);nm=ok('nm-path',['/usr/bin/xcrun','--find','nm']);resource=Path(ok('resource',[clang,'-print-resource-dir'])).resolve(strict=True)
  mac=Path(ok('mac-sdk',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'])).resolve(strict=True);ios=Path(ok('iphone-sdk',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-path'])).resolve(strict=True);version=ok('iphone-version',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-version']);assert re.fullmatch(r'\d+\.\d+(?:\.\d+)?',version)and int(version.split('.')[0])>=27
  metadata['tools']=dict(clang=pin(Path(clang)),ar=pin(Path(ar)),nm=pin(Path(nm)),iphoneSDKVersion=version,hostMachine=platform.machine(),hostSystem=sys.platform)
  flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-g0','-D_DARWIN_C_SOURCE=1','-fvisibility=hidden','-fno-builtin','-fno-stack-protector','-fno-lto','-fstack-usage','-I',str(source)]
  def compile(unit,label,target,sdk,extra=(),inputFile=None,directory=None):
   directory=directory or out/'mac';directory.mkdir(exist_ok=True);obj=directory/(label+'.o');p=run('compile-'+label,[clang,*flags,'-target',target,'-isysroot',sdk,*extra,'-MD','-MF',obj.with_suffix('.d'),'-c',inputFile or source/unit,'-o',obj]);assert p.returncode==0
   deptext=obj.with_suffix('.d').read_text().replace('\\\n',' ');deps=shlex.split(deptext.split(':',1)[1]);assert 0<len(deps)<=1024
   for n in deps:
    path=Path(n).resolve(strict=True);assert any(path.is_relative_to(root)for root in(source,sdk,resource))or(inputFile is not None and path==inputFile.resolve(strict=True))
   ownframes={}
   for line in obj.with_suffix('.su').read_text().splitlines():
    where,size,kind=line.split('\t');assert kind=='static'and 0<=int(size)<=1048576;ownframes[where.rsplit(':',1)[1]]=int(size)
   frames[str(obj.relative_to(out))]=dict(ownStaticBytesExcludingCallees=ownframes,source=unit,dependencies=len(deps));return obj
  common=[compile(u,Path(u).stem,'arm64-apple-macos14.0',mac)for u in PRODUCTION]
  env=compile('synthetic_environment.c','synthetic-environment','arm64-apple-macos14.0',mac);fixture=compile('fixture_pthread.c','fixture','arm64-apple-macos14.0',mac);testing=compile(PRODUCTION[0],'testing-provider','arm64-apple-macos14.0',mac,['-DNGP_FLS_TESTING=1']);tf=compile('fixture_pthread.c','testing-fixture','arm64-apple-macos14.0',mac,['-DNGP_FLS_TESTING=1'])
  def link(label,provider,f):
   exe=out/'mac'/label;ok('link-'+label,[clang,'-target','arm64-apple-macos14.0','-isysroot',mac,provider,*common[1:],env,f,'-o',exe]);raw=exe.read_bytes();assert struct.unpack_from('<IIII',raw)==(0xfeedfacf,0x100000c,0,2);return exe
  binaries={'production':link('production',common[0],fixture),'testing':link('testing',testing,tf)}
  original=(source/PRODUCTION[0]).read_bytes();needle=b'rc=s->callback.function(s->callback.context,b->value);';foreign=b'if(ngp_worker_tag_read_v1(&tag)||tag!=s->worker_tag)return emit(s,r,NGP_FLS_OWNER,0);'
  mutants={'skip-callback':(needle,b'rc=0;'),'duplicate-callback':(needle,needle+b'if(!rc)'+needle),'foreign-tracking':(foreign,foreign.replace(b'NGP_FLS_OWNER,0',b'NGP_FLS_OWNER,1'))}
  for label,(before,after)in mutants.items():
   assert original.count(before)==1;changed=original.replace(before,after);assert changed.replace(after,before)==original;path=out/'mac'/(label+'.c');path.write_bytes(changed);obj=compile(PRODUCTION[0],label,'arm64-apple-macos14.0',mac,inputFile=path);binaries[label]=link(label,obj,fixture)
  schedule=json.loads((source/'schedule.json').read_bytes());prod=('normal','capacity','owner-exit','callback-fail');test=('create-fail','set-fail','clear-fail','delete-fail','late-exit');assert set(schedule)==set(prod+test)
  for binary,mode in[('production',m)for m in prod]+[('testing',m)for m in test]+[(n,'normal')for n in mutants]:
   stable();p=run('native-'+binary+'-'+mode,[binaries[binary],mode],30);text=p.stdout.decode();err=p.stderr.decode()
   if binary in('production','testing'):
    row=schedule[mode];assert p.returncode==0 and not err and re.fullmatch(r'PASS native-pthread-foundation mode='+re.escape(mode)+r' checks='+str(row['checks'])+r' callbacks='+str(row['callbacks'])+r' syntheticHost=1 actualPthreads=1 retainedBoxes=64\r?\n',text)
   else:
    assert p.returncode==4 and not text and re.fullmatch(r'FAIL line\d+ .+\r?\n',err)
    if binary=='foreign-tracking':assert'ngp_fls_get_v1(f->service,f->token,&out,&r)==NGP_FLS_OWNER'in err and'!r.worker_observation'in err and'!r.boxes_used'in err
    else:assert'ngp_fls_retire_v1(f->service,f->token,&r)==0'in err and'atomic_load(&callbacks)==1'in err
   completed.append(dict(binary=binary,mode=mode,exitCode=p.returncode,receipt=text or err));stable()
  assert sum(schedule[m]['checks']for m in prod+test)==519
  byunit={};archiveObjects=[]
  for repeat in('primary','repeat'):
   rows=[]
   for unit in PRODUCTION:
    obj=compile(unit,repeat+'-'+Path(unit).stem,'arm64-apple-ios26.0',ios,directory=out/repeat);raw=obj.read_bytes();gate='inspect-'+repeat+'-'+Path(unit).stem;row=parse(raw,unit);rows.append(row)
    if unit==PRODUCTION[0]:
     layout=run('layout-'+repeat,[clang,*flags,'-target','arm64-apple-ios26.0','-isysroot',ios,'-Xclang','-fdump-record-layouts-complete','-fsyntax-only',source/unit]);assert layout.returncode==0;gate='inspect-layout-'+repeat;blocks=[b for b in layout.stdout.decode().split('*** Dumping AST Record Layout')if'owner_exited'in b];assert len(blocks)==1;block=blocks[0];m=re.search(r'^\s*(\d+) \|\s+.+\bowner_exited$',block,re.M);size=re.search(r'\[sizeof=(\d+), align=(\d+)\]',block);assert m and size
     row['actualPrivateLayout']=dict(bytes=int(size[1]),alignment=int(size[2]),ownerExitOffset=int(m[1]));gate='inspect-destructor-'+repeat;row['destructor']=destructor(raw,row['ownerExitSpan'],int(m[1]))
    row['object']=pin(obj);row['relativePath']=str(obj.relative_to(out));
    if repeat=='primary':byunit[unit]=obj;archiveObjects.append(obj)
    else:assert raw==byunit[unit].read_bytes()
   metadata[repeat]=rows
  imported=set().union(*(set(r['undefined'])for r in metadata['primary']));exports=set().union(*REQUIRED.values());external=imported-exports;assert ENV<=external and external<=ENV|SYSTEM
  assert {'_pthread_main_np','_pthread_key_create','_pthread_key_delete','_pthread_getspecific','_pthread_setspecific','_pthread_setcancelstate'}<=external
  archive=out/'libngp_fls.a';ok('archive',[ar,'-crs',archive,*archiveObjects]);names=ok('archive-symbols',[nm,'-gU',archive]);observed=re.findall(r'^[0-9a-fA-F]+ [A-Za-z] (\S+)$',names,re.M);assert set(observed)==exports and len(observed)==13;metadata['archive']=pin(archive);metadata['unboundOwnEnvironment']=sorted(ENV);metadata['systemImports']=sorted(external-ENV)
  stable();status='passed'
 except BaseException as exc:
  failedType=type(exc).__name__;ownframes=[dict(file=Path(t.filename).name,line=t.lineno,function=t.name)for t in traceback.extract_tb(exc.__traceback__)if Path(t.filename).resolve().parent==source][-4:]
  failureDetail=json.dumps(dict(type=failedType,message=str(exc)[:400],ownFrames=ownframes,lastCommand=commands[-1]['label']if commands else None,lastExitCode=commands[-1]['exitCode']if commands else None),ensure_ascii=True,separators=(',',':'))[:1100]
 finally:
  result=dict(schema='native-pthread-foundation-actual-apple-v1',status=status,failedGate=None if status=='passed'else gate,failureType=failedType,failureDetail=failureDetail,retirementUncertain=retirementUncertain,completed=completed,actualMacRuntimeRequired=True,syntheticHost=True,gameHostBindingProved=False,iphoneRuntimeProved=False,SDKCompileProved=status=='passed',toolDescendantDrainProved=False)
  save(out/'result.json',result);save(out/'source-identities.json',closure);save(out/'commands.json',commands);save(out/'metadata.json',metadata);save(out/'frames.json',frames)
 print(json.dumps(dict(status=status,failedGate=result['failedGate'],completedModes=len(completed))))
 return 0 if status=='passed'else 1
if __name__=='__main__':raise SystemExit(main())
