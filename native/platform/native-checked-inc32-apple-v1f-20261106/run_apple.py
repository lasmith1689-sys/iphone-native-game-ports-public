"""Actual arm64 Mac synthetic runtime + entire own iPhone SDK cohort only."""
import argparse,hashlib,json,os,platform,re,shlex,struct,subprocess,sys,threading,time,traceback
from pathlib import Path
sys.dont_write_bytecode=True
from inspect_sdk import SOURCES,PRODUCTION,COHORT,REQUIRED,parse,bind_frames,archive,SYSTEM
def pin(p):
 b=p.read_bytes();return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args();source=Path(__file__).resolve().parent;assert sys.platform=='darwin'and platform.machine()=='arm64','actual Darwin arm64 host required before output creation';out=args.output.resolve();assert out.parent==source.parent and out.name=='output'and not out.exists()and not source.is_symlink();out.mkdir();(out/'logs').mkdir();commands=[];frames={};metadata={};closure={};completed=[];started=time.monotonic();gate='source';retirementUncertain=False;status='failed';failedType=None;failureDetail=None;toolPaths={};toolBefore={};selectedDeps={}
 def stable():assert set(p.name for p in source.iterdir())==set(SOURCES)and all(not(source/n).is_symlink()and pin(source/n)==closure[n]for n in SOURCES)
 def register_tool(label,path):
  path=Path(path).resolve(strict=True);assert path.is_file()and label not in toolPaths;toolPaths[label]=path;toolBefore[label]=pin(path)
 def tools_stable(argv=None):
  selected=list(toolPaths)
  if argv is not None:
   actual=Path(argv[0]).resolve(strict=True);selected=[n for n,p in toolPaths.items()if p==actual];assert len(selected)==1,'unregistered execution input'
  for n in selected:assert pin(toolPaths[n])==toolBefore[n],('execution input changed',n)
 def headers_stable(selected=None):
  for path,want in (selectedDeps if selected is None else selected).items():assert pin(Path(path))==want,'selected compile dependency changed'
 def dep_paths(raw):
  text=raw.decode().replace('\\\n',' ');parts=shlex.split(text.split(':',1)[1]);assert 0<len(parts)<=1024;paths=[Path(n).resolve(strict=True)for n in parts];assert len(paths)==len(set(paths));return paths
 def run(label,argv,timeout=60):
  nonlocal gate,retirementUncertain
  assert re.fullmatch('[a-z0-9_-]{1,96}',label);gate=label;remaining=600-(time.monotonic()-started);assert remaining>0;limit=min(timeout,remaining);begin=time.monotonic();tools_stable(argv);errors=[];overflow=[False];streams={};threads=[];child=None;reaped=False;timedout=False;code=None
  for stream in('stdout','stderr'):(out/'logs'/(label+'.'+stream)).write_bytes(b'')
  def pump(name,pipe):
   seen=stored=0
   try:
    with(out/'logs'/(label+'.'+name)).open('wb',buffering=0)as f:
     while True:
      b=pipe.read(4096)
      if not b:break
      seen+=len(b);keep=b[:max(0,262144-stored)];f.write(keep);stored+=len(keep)
      if seen>262144:overflow[0]=True;break
     os.fsync(f.fileno())
   except BaseException as exc:errors.append(type(exc).__name__)
   finally:
    streams[name]=dict(seen=seen,stored=stored)
    try:pipe.close()
    except BaseException as exc:errors.append(type(exc).__name__)
  try:
   child=subprocess.Popen(list(map(str,argv)),cwd=out,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env={**os.environ,'ZERO_AR_DATE':'1','LC_ALL':'C','PYTHONDONTWRITEBYTECODE':'1'});threads=[threading.Thread(target=pump,args=(n,p),daemon=True)for n,p in(('stdout',child.stdout),('stderr',child.stderr))]
   for t in threads:t.start()
   while child.poll()is None:
    if errors or overflow[0]or time.monotonic()-begin>=limit:timedout=time.monotonic()-begin>=limit;child.kill();break
    time.sleep(.02)
   code=child.wait(timeout=5);reaped=True
  except BaseException as exc:
   errors.append(type(exc).__name__)
   if child is not None:
    try:
     if child.poll()is None:child.kill()
     code=child.wait(timeout=5);reaped=True
    except BaseException as cleanup:errors.append(type(cleanup).__name__)
  for t in threads:t.join(timeout=1)
  readersRetired=not any(t.is_alive()for t in threads);bad=not reaped or not readersRetired or bool(errors)or overflow[0]or timedout
  if bad:retirementUncertain=True
  commands.append(dict(label=label,argv=list(map(str,argv)),exitCode=code,elapsedSeconds=time.monotonic()-begin,timeout=timedout,childStarted=child is not None,childReaped=reaped,readerThreadsRetired=readersRetired,captureErrors=list(errors),outputOverflow=overflow[0],retirementUncertain=bad,readerStreams=streams,stdout=pin(out/'logs'/(label+'.stdout')),stderr=pin(out/'logs'/(label+'.stderr'))));save(out/'commands.json',commands)
  assert not bad,'bounded owned command/capture incomplete';tools_stable(argv)
  stdout=(out/'logs'/(label+'.stdout')).read_bytes();stderr=(out/'logs'/(label+'.stderr')).read_bytes();assert len(stdout)<=262144 and len(stderr)<=262144;return subprocess.CompletedProcess(argv,code,stdout,stderr)
 def ok(label,argv):p=run(label,argv);assert p.returncode==0;return p.stdout.decode().strip()
 try:
  closure={n:pin(source/n)for n in SOURCES};stable();gate='tools';register_tool('python',sys.executable);register_tool('xcrun','/usr/bin/xcrun')
  clang=ok('clang-path',['/usr/bin/xcrun','--find','clang']);register_tool('clang',clang);ar=ok('ar-path',['/usr/bin/xcrun','--find','ar']);register_tool('ar',ar);nm=ok('nm-path',['/usr/bin/xcrun','--find','nm']);register_tool('nm',nm);resource=Path(ok('resource',[clang,'-print-resource-dir'])).resolve(strict=True);mac=Path(ok('mac-sdk',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'])).resolve(strict=True);ios=Path(ok('iphone-sdk',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-path'])).resolve(strict=True);version=ok('iphone-version',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-version']);assert version=='27.1';metadata['tools']=dict(clang=pin(Path(clang)),ar=pin(Path(ar)),nm=pin(Path(nm)),iphoneSDKVersion=version,actualHostMachine=platform.machine(),actualHostSystem=sys.platform)
  flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-g0','-D_DARWIN_C_SOURCE=1','-fvisibility=hidden','-fno-builtin','-fno-stack-protector','-fno-lto','-funwind-tables','-ffp-contract=off','-fno-fast-math','-fno-strict-aliasing','-fstack-usage','-I',str(source)]
  def compile(unit,label,target,sdk,directory,inputFile=None):
   directory.mkdir(exist_ok=True);obj=directory/(label+'.o');actualInput=inputFile or source/unit;headers_stable();preFlags=[f for f in flags if f!='-fstack-usage'];preflight=run('deps-'+label,[clang,*preFlags,'-target',target,'-isysroot',sdk,'-M','-MT',label+'.o',actualInput]);assert preflight.returncode==0;paths=dep_paths(preflight.stdout);headers=[];beforeDeps={}
   for path in paths:
    roots=[('own',source),('sdk',sdk),('clang',resource)];matches=[(role,root)for role,root in roots if path.is_relative_to(root)]
    if inputFile is not None and path==inputFile.resolve(strict=True):role='own-mutant';relative=inputFile.name
    else:assert len(matches)==1;role,root=matches[0];relative=path.relative_to(root).as_posix()
    assert path.stat().st_size<=4194304;identity=pin(path);key=str(path)
    if key in selectedDeps:assert selectedDeps[key]==identity
    selectedDeps[key]=identity;beforeDeps[key]=identity;headers.append(dict(domain=role,path=relative,identity=identity))
   assert any(r['domain']in('own','own-mutant')and r['path']==actualInput.name for r in headers);headers_stable(beforeDeps);p=run('compile-'+label,[clang,*flags,'-target',target,'-isysroot',sdk,'-MD','-MF',obj.with_suffix('.d'),'-c',actualInput,'-o',obj]);headers_stable(beforeDeps);assert p.returncode==0;actual=dep_paths(obj.with_suffix('.d').read_bytes());assert set(map(str,actual))==set(beforeDeps),'preflight/actual dependency roster changed';ownframes={}
   for line in obj.with_suffix('.su').read_text().splitlines():where,n,kind=line.split('\t');assert kind=='static'and 0<=int(n)<=1048576;name=where.rsplit(':',1)[1];assert name not in ownframes;ownframes[name]=int(n)
   frames[obj.relative_to(out).as_posix()]=dict(ownStaticBytesExcludingCallees=ownframes,source=unit,actualHeaderDependencies=headers,dependencyPreflightGenuine=True,allCompileDependencyBytesEqualBeforeAfter=True,actualMDMatchesPreflightExactly=True,machinePrologueCostProved=False);return obj
  common={u:compile(u,Path(u).stem,'arm64-apple-macos14.0',mac,out/'mac')for u in COHORT}
  def link(label,replacements):
   exe=out/'mac'/label;ok('link-'+label,[clang,'-target','arm64-apple-macos14.0','-isysroot',mac,*[replacements.get(u,common[u])for u in COHORT],'-o',exe]);assert struct.unpack_from('<4I',exe.read_bytes())==(0xfeedfacf,0x100000c,0,2);register_tool('native-'+label,exe);return exe
  binaries={'production':link('production',{})};changes={
   'wrong-cf':('inc32_publish.c','cpu->af=flags.af;','cpu->af=flags.af;cpu->cf=0;'),
   'wrong-af':('inc32_publish.c','cpu->af=flags.af;','cpu->af=0;'),
   'wrong-width':('native_atomic_required.c','address,4,PC64_XADD,0,1,&old','address,8,PC64_XADD,0,1,&old'),
   'premature-flags':('native_atomic_required.c','uint64_t old=0;q=pc64_host_atomic','pc64_inc32_publish_v1(cpu,0);uint64_t old=0;q=pc64_host_atomic'),
   'stale-preread':('native_atomic_required.c','uint64_t old=0;q=pc64_host_atomic(s->host,address,4,PC64_XADD,0,1,&old);if(q)source_fail(cpu,q,sourceVA,address,4);\n pc64_inc32_publish_v1(cpu,old);','extern uint32_t pc64_test_preread_v1(uint64_t);uint32_t stale=pc64_test_preread_v1(address);uint64_t old=0;q=pc64_host_atomic(s->host,address,4,PC64_XADD,0,1,&old);if(q)source_fail(cpu,q,sourceVA,address,4);\n pc64_inc32_publish_v1(cpu,stale);'),
   'split-rmw':('host64.c','previous=__atomic_fetch_add(ptr,(uint32_t)value,__ATOMIC_SEQ_CST);','previous=pc64_test_split_fetch_add(ptr,(uint32_t)value);'),
   'wrong-marker':('synthetic_profiles.c','UINT64_C(0x10001000),UINT64_C(0x10000100)','UINT64_C(0x10001000),UINT64_C(0x10000101)')}
  for label,(unit,before,after)in changes.items():
   original=(source/unit).read_bytes();assert original.count(before.encode())==1;body=original.replace(before.encode(),after.encode());prefix=b'#include <stdint.h>\nextern uint32_t pc64_test_split_fetch_add(uint32_t*,uint32_t);\n'if label=='split-rmw'else b'';body=prefix+body;assert body[len(prefix):].replace(after.encode(),before.encode())==original;path=out/'mac'/(label+'.c');path.write_bytes(body);obj=compile(unit,label,'arm64-apple-macos14.0',mac,out/'mac',path);binaries[label]=link(label,{unit:obj})
  schedule=json.loads((source/'schedule.json').read_bytes());cases=[('production',m)for m in schedule['positive']]+[(n,r['mode'])for n,r in schedule['negative'].items()];assert len(cases)==16
  for binary,mode in cases:
   stable();p=run('native-'+binary+'-'+mode,[binaries[binary],mode],30);text=p.stdout.decode();err=p.stderr.decode()
   if binary=='production':
    row=schedule['positive'][mode];wanted='PASS checked-inc32 mode='+mode+' '+' '.join(f'{n}={v}'for n,v in row.items())+' mainRefused=1 actualPthreads=1 syntheticInputs=1 sourceReady=0';assert p.returncode==0 and not err and re.fullmatch(re.escape(wanted)+r'\r?\n',text)
   else:
    row=schedule['negative'][binary];assert p.returncode==4 and not text and re.fullmatch(re.escape('FAIL line'+str(row['line'])+' '+row['expression'])+r'\r?\n',err)
   completed.append(dict(binary=binary,mode=mode,exitCode=p.returncode,receipt=text or err));stable()
  metadata['primary']=[];metadata['repeat']=[];primary={};primaryMeta={}
  for repeat in('primary','repeat'):
   for unit in COHORT:
    obj=compile(unit,repeat+'-'+Path(unit).stem,'arm64-apple-ios26.0',ios,out/repeat);raw=obj.read_bytes();gate='inspect-'+repeat+'-'+Path(unit).stem;row=parse(raw,unit);row['actualSourceHash']=closure[unit];row['relativePath']=obj.relative_to(out).as_posix();row['sourceBoundOwnFrames']=bind_frames(row,frames[row['relativePath']]['ownStaticBytesExcludingCallees']);metadata[repeat].append(row)
    if repeat=='primary':primary[unit]=obj;primaryMeta[obj.name]=row
    else:assert raw==primary[unit].read_bytes()
  external=set().union(*(set(r['undefined'])for r in metadata['primary']))-set().union(*(set(v)for v in REQUIRED.values()));assert external<=SYSTEM;assert {'_pthread_main_np','_pthread_create','_pthread_join','_setjmp','_longjmp','__tlv_bootstrap'}<=external;metadata['systemImports']=sorted(external)
  productionObjects={primary[u].name:primary[u].read_bytes()for u in PRODUCTION};productionMeta={primary[u].name:primaryMeta[primary[u].name]for u in PRODUCTION};archivePath=out/'libpc64_checked_inc32_own.a';ok('archive',[ar,'-crs',archivePath,*[primary[u]for u in PRODUCTION]]);gate='inspect-archive';metadata['archive']=archive(archivePath.read_bytes(),productionObjects,productionMeta);names=ok('archive-symbols',[nm,'-gU',archivePath]);observed=re.findall(r'^[0-9a-fA-F]+ [A-Za-z] (\S+)$',names,re.M);expected=set().union(*(set(REQUIRED[u])for u in PRODUCTION));assert set(observed)==expected and len(observed)==50;metadata['CPUHeaderCompileAssertionsAccepted']=True;metadata['allEightOwnUnitsRepeatedByteexact']=True;stable();tools_stable();headers_stable();metadata['allSelectedExecutionInputBytesEqualBeforeAfter']=True;metadata['allSelectedCompileDependencyBytesEqualBeforeAfter']=True;status='passed'
 except BaseException as exc:
  failedType=type(exc).__name__;ownframes=[dict(file=Path(t.filename).name,line=t.lineno,function=t.name)for t in traceback.extract_tb(exc.__traceback__)if Path(t.filename).resolve().parent==source][-4:];failureDetail=json.dumps(dict(type=failedType,message=str(exc)[:400],ownFrames=ownframes,lastCommand=commands[-1]['label']if commands else None,lastExitCode=commands[-1]['exitCode']if commands else None),ensure_ascii=True,separators=(',',':'))[:1100]
 finally:
  def final_measure(path):
   try:return pin(path)
   except OSError as problem:return dict(unavailable=type(problem).__name__)
  metadata['executionInputIdentitiesBefore']=toolBefore;metadata['executionInputIdentitiesAfter']={n:final_measure(p)for n,p in toolPaths.items()};metadata['compileDependencyIdentitiesBefore']=selectedDeps;metadata['compileDependencyIdentitiesAfter']={n:final_measure(Path(n))for n in selectedDeps};metadata['selectedCompileDependencyCount']=len(selectedDeps)
  finalEqual=metadata['executionInputIdentitiesBefore']==metadata['executionInputIdentitiesAfter']and metadata['compileDependencyIdentitiesBefore']==metadata['compileDependencyIdentitiesAfter']
  if status=='passed'and not finalEqual:status='failed';gate='final-input-identities';failedType='AssertionError';failureDetail='selected execution/header input bytes changed after work/readback'

  result=dict(schema='own-checked-inc32-actual-apple-v1',status=status,failedGate=None if status=='passed'else gate,failureType=failedType,failureDetail=failureDetail,retirementUncertain=retirementUncertain,completed=completed,actualMacRuntimeRequired=True,syntheticHost=True,syntheticInputs=True,gameHostBindingProved=False,iphoneRuntimeProved=False,SDKCompileProved=status=='passed',toolDescendantDrainProved=False);save(out/'result.json',result);save(out/'source-identities.json',closure);save(out/'commands.json',commands);save(out/'metadata.json',metadata);save(out/'frames.json',frames)
 print(json.dumps(dict(status=status,failedGate=result['failedGate'],completedModes=len(completed))));return 0 if status=='passed'else 1
if __name__=='__main__':raise SystemExit(main())
