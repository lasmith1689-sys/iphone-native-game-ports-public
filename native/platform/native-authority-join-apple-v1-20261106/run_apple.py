"""Actual OWN host/dispatcher/FLS/INC/tag join, genuine Mac and unsigned SDK only."""
import argparse,hashlib,json,math,os,platform,re,shlex,struct,subprocess,sys,threading,time,traceback
from pathlib import Path
sys.dont_write_bytecode=True
from inspect_sdk import SOURCES,PRODUCTION,COHORT,REQUIRED,parse,bind_frames,archive,SYSTEM,image,linkmap
def pin(p):
    b=p.read_bytes();return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def strict(p):
    def pairs(items):
        d={}
        for k,v in items:
            if k in d:raise ValueError('duplicate JSON key')
            d[k]=v
        return d
    def bad(value):raise ValueError('nonfinite JSON')
    def real(value):
        n=float(value)
        if not math.isfinite(n):raise ValueError('nonfinite JSON')
        return n
    return json.loads(p.read_bytes(),object_pairs_hook=pairs,parse_constant=bad,parse_float=real)
def identity_after(paths):
    identities={};errors=[]
    for name,path in paths.items():
        try:identities[name]=pin(path)
        except (OSError,ValueError)as exc:errors.append(dict(input=name,failureType=type(exc).__name__))
    return identities,errors

def artifact_closure(out,paths):
    selected={};errors=[]
    for i,p in enumerate(paths):
        try:
            if not p.is_relative_to(out)or p.is_symlink():raise ValueError('outside owned regular closure')
            name=p.relative_to(out).as_posix()
            if name in selected:raise ValueError('duplicate selected file')
            if not p.is_file():raise FileNotFoundError('selected file absent')
            row=pin(p)
            if row['bytes']>8388608:raise ValueError('selected file cap')
            selected[name]=row
        except (OSError,ValueError)as exc:errors.append(dict(ordinal=i,failureType=type(exc).__name__))
    if len(selected)>512 or sum(r['bytes']for r in selected.values())>33554432:
        errors.append(dict(ordinal=len(paths),failureType='OutputClosureCap'))
    return selected,errors

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
    source=Path(__file__).resolve().parent
    assert sys.platform=='darwin'and platform.machine()=='arm64','actual Darwin arm64 required before output creation'
    out=args.output.resolve();assert out.parent==source.parent and out.name=='output'and not out.exists()and not source.is_symlink()
    out.mkdir();(out/'logs').mkdir();commands=[];frames={};metadata={};closure={};completed=[]
    started=time.monotonic();gate='source';retirementUncertain=False;status='failed';failureType=None;failureDetail=None
    toolPaths={};toolBefore={};selectedDeps={};retained=[];objects={};objectRows={};macImages={};executables={};mutants={};sdkRows={'primary':[],'repeat':[]}
    archiveRow=None;sdkImage=None;sdkMap=None
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
        closure={n:pin(source/n)for n in SOURCES};stable();save(out/'source-identities.json',closure)
        plan=strict(source/'schedule_join.json');assert len(plan['positive'])==10 and len(plan['negative'])==5
        register_tool('python',Path(sys.executable));register_tool('xcrun',Path('/usr/bin/xcrun'))
        clang=Path(ok('clang-path',['/usr/bin/xcrun','--find','clang'])).resolve(strict=True);register_tool('clang',clang)
        ar=Path(ok('ar-path',['/usr/bin/xcrun','--find','ar'])).resolve(strict=True);register_tool('ar',ar)
        nm=Path(ok('nm-path',['/usr/bin/xcrun','--find','nm'])).resolve(strict=True);register_tool('nm',nm)
        resource=Path(ok('resource',[clang,'-print-resource-dir'])).resolve(strict=True)
        mac=Path(ok('mac-sdk',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'])).resolve(strict=True)
        ios=Path(ok('iphone-sdk',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-path'])).resolve(strict=True)
        assert ok('iphone-version',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-version'])=='27.1'
        assert ok('mac-version',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-version'])=='27.1'
        roots=(('own',source),('own-control',out/'mutants'),('mac-sdk',mac),('iphone-sdk',ios),('clang',resource))
        for sdk in(mac,ios):
            p=sdk/'SDKSettings.json';assert p.is_file()and not p.is_symlink();selectedDeps[str(p)]=pin(p)
        flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-g0','-D_DARWIN_C_SOURCE=1','-fvisibility=hidden','-fno-builtin','-fno-stack-protector','-fno-lto','-funwind-tables','-ffp-contract=off','-fno-fast-math','-fno-strict-aliasing','-fstack-usage','-I',str(source)]
        objects={};objectRows={};retained=[];mutants={}
        def dependency(p):
            matches=[(name,root)for name,root in roots if p.is_relative_to(root)]
            assert len(matches)==1,('outside selected input domains',p.name)
            domain,root=matches[0];return dict(domain=domain,path=p.relative_to(root).as_posix(),identity=pin(p))
        def compile_unit(unit,label,target,sdk,directory,extra=(),actual=None,variant='baseline'):
            nonlocal gate
            stable();directory.mkdir(exist_ok=True);path=directory/(label+'.o');actual=actual or(source/unit)
            preFlags=[x for x in flags if x!='-fstack-usage'];headers_stable()
            p=run('deps-'+label,[clang,*preFlags,*extra,'-target',target,'-isysroot',sdk,'-M','-MT','unit.o',actual]);assert p.returncode==0
            paths=dep_paths(p.stdout);before={str(x):pin(x)for x in paths};assert str(actual.resolve())in before
            for name,want in before.items():
                if name in selectedDeps:assert selectedDeps[name]==want
                selectedDeps[name]=want
            deps=[dependency(x)for x in paths];headers_stable(before)
            p=run('compile-'+label,[clang,*flags,*extra,'-target',target,'-isysroot',sdk,'-MD','-MF',path.with_suffix('.d'),'-MT','unit.o','-c',actual,'-o',path]);headers_stable(before);assert p.returncode==0
            assert set(map(str,dep_paths(path.with_suffix('.d').read_bytes())))==set(before)
            su=path.with_suffix('.su');assert su.is_file()and not su.is_symlink()and su.stat().st_size<=1048576
            local={}
            for line in su.read_text().splitlines():
                parts=line.split('\t');assert len(parts)==3 and parts[2]=='static';name=parts[0].rsplit(':',1)[1];assert name not in local;local[name]=int(parts[1]);assert 0<=local[name]<=1048576
            for p in(path,path.with_suffix('.d'),su):retained.append(p)
            gate='inspect-'+label;raw=path.read_bytes();profile='iphone'if sdk==ios else'mac';row=parse(raw,unit,profile,variant)
            row.update(relativePath=path.relative_to(out).as_posix(),actualInput=actual.name,actualSourceHash=pin(actual),variant=variant,sourceBoundOwnFrames=bind_frames(row,local))
            frames[row['relativePath']]=dict(source=unit,rawSU=pin(su),rawDependencyList=pin(path.with_suffix('.d')),ownStaticBytesExcludingCallees=local,machinePrologueCostProved=False,actualMDMatchesPreflightExactly=True,actualHeaderDependencies=deps)
            objectRows[str(path)]=row;objects[label]=path;return path
        common={u:compile_unit(u,Path(u).stem,'arm64-apple-macos14.0',mac,out/'mac')for u in COHORT}
        testing={u:compile_unit(u,'testing-'+Path(u).stem,'arm64-apple-macos14.0',mac,out/'mac',('-DNGP_FLS_TESTING=1',),variant='fls-testing'if u=='ngp_fls_v1.c'else'fls-testing-fixture')for u in('ngp_fls_v1.c','fixture_authority_join.c')}
        (out/'mutants').mkdir()
        def changed(name,unit,before,after):
            old=(source/unit).read_bytes();a=before.encode();b=after.encode();assert old.count(a)==1
            new=old.replace(a,b);assert new.replace(b,a)==old;path=out/'mutants'/(name+'.c');path.write_bytes(new)
            mutants[name]=dict(unit=unit,sourceIdentity=pin(source/unit),controlIdentity=pin(path),before=before,after=after,inverseExact=True);retained.append(path);return path
        controlSource={
            'independent-parent':('ngp_worker_tag_bridge_pc64_v1.c',source/'independent_tag_control.c'),
            'zero-depth':('ngp_fls_environment_pc64_v1.c',changed('zero-depth','ngp_fls_environment_pc64_v1.c','return pc64_dispatch_depth();','return 0;')),
            'wrong-host':('ngp_fls_environment_pc64_v1.c',changed('wrong-host','ngp_fls_environment_pc64_v1.c','return pc64_active_host();','return NULL;')),
            'skip-callback':('ngp_fls_v1.c',changed('skip-callback','ngp_fls_v1.c','rc=s->callback.function(s->callback.context,b->value);if(rc)return fail(s,r,NGP_FLS_CALLBACK,rc);','rc=0;')),
            'fake-atomic':('host64.c',changed('fake-atomic','host64.c','__atomic_fetch_add(ptr,(uint32_t)value,__ATOMIC_SEQ_CST)','__atomic_load_n(ptr,__ATOMIC_SEQ_CST)'))}
        controls={name:(unit,compile_unit(unit,name,'arm64-apple-macos14.0',mac,out/'mac',actual=actual,variant='control'))for name,(unit,actual)in controlSource.items()}
        def expected(replacements):
            required={};owners={}
            for unit in COHORT:
                obj=replacements.get(unit,common[unit]);row=objectRows[str(obj)]
                for name,r in row['requiredFunctions'].items():assert name not in required;required[name]=r['kind'];owners[name]=obj.name
            return required,owners
        macImages={};executables={}
        def link_mac(name,replacements):
            nonlocal gate
            exe=out/'mac'/name;mapping=exe.with_suffix('.map');chosen=[replacements.get(u,common[u])for u in COHORT]
            ok('link-'+name,[clang,'-target','arm64-apple-macos14.0','-isysroot',mac,*chosen,'-Wl,-no_deduplicate','-Wl,-map,'+str(mapping),'-o',exe])
            retained.extend((exe,mapping));gate='inspect-link-'+name;req,owners=expected(replacements);parsed=image(exe.read_bytes(),'mac',req,2);joined=linkmap(mapping.read_bytes(),parsed,owners)
            macImages[name]=dict(relativePath=exe.relative_to(out).as_posix(),rawMap=mapping.relative_to(out).as_posix(),metadata=parsed,wholeObjectJoin=joined)
            register_tool('native-'+name,exe);executables[name]=exe
        link_mac('production',{});link_mac('testing',testing)
        for name,(unit,obj)in controls.items():link_mac(name,{unit:obj})
        def native(name,mode,negative=False):
            nonlocal gate
            p=run('native-'+name+'-'+mode,[executables[name],mode],30)
            if negative:
                wanted=plan['negative'][name];assert p.returncode==wanted['exitCode']and not p.stdout
                receipt=p.stderr.decode();assert receipt=='FAIL line'+str(wanted['line'])+' '+wanted['expression']+'\n'
            else:
                wanted=plan['positive'][mode];assert name==wanted['binary']and p.returncode==0 and not p.stderr
                receipt=p.stdout.decode();m=re.fullmatch(r'PASS authority-join mode=([a-z-]+) cases=(\d+) faults=(\d+) foreignOps=(\d+) callbacks=(\d+) boxes=(\d+) lateEvent=(\d+) tagThreads=2 realHost=1 realDepth=1 ownCallback=1 sourceReady=0\n',receipt)
                assert m and m[1]==mode and list(map(int,m.groups()[1:]))==[wanted[k]for k in('cases','faults','foreignOps','callbacks','boxes','lateEvent')]
            completed.append(dict(binary=name,mode=mode,exitCode=p.returncode,receipt=receipt,negative=negative));save(out/'result.json',dict(status='in-progress',completed=completed,retirementUncertain=retirementUncertain,toolDescendantDrainProved=False))
        # Deliberate real independent-allocator RED precedes acceptance of the
        # first shared-allocator GREEN. It must reject by its fixed assertion.
        native('independent-parent','normal',True)
        for mode,wanted in plan['positive'].items():native(wanted['binary'],mode)
        for name in plan['negative']:
            if name!='independent-parent':native(name,'normal',True)
        primary={};sdkRows={'primary':[],'repeat':[]}
        for phase in('primary','repeat'):
            for unit in COHORT:
                obj=compile_unit(unit,phase+'-'+Path(unit).stem,'arm64-apple-ios26.0',ios,out/phase)
                raw=obj.read_bytes()
                if phase=='primary':primary[unit]=obj
                else:assert raw==primary[unit].read_bytes()
                sdkRows[phase].append(objectRows[str(obj)])
        archivePath=out/'libnative_authority_join.a'
        ok('archive',[ar,'-crs',archivePath,*[primary[u]for u in PRODUCTION]]);retained.append(archivePath)
        ok('archive-symbols',[nm,'-gU',archivePath])
        productionObjects={primary[u].name:primary[u].read_bytes()for u in PRODUCTION};productionMeta={primary[u].name:objectRows[str(primary[u])]for u in PRODUCTION}
        gate='inspect-archive';archiveRow=archive(archivePath.read_bytes(),productionObjects,productionMeta);assert len(archiveRow['symbols'])==66
        lib=out/'libnative_authority_join_own.dylib';mapping=out/'sdk-whole.map'
        ok('sdk-link',[clang,'-target','arm64-apple-ios26.0','-isysroot',ios,'-dynamiclib',*[primary[u]for u in COHORT],'-Wl,-no_adhoc_codesign','-Wl,-no_deduplicate','-install_name','@rpath/libnative_authority_join_own.dylib','-Wl,-map,'+str(mapping),'-o',lib])
        retained.extend((lib,mapping));gate='inspect-sdk-whole';required={n:k for u in COHORT for n,k in REQUIRED[u].items()};owners={n:primary[u].name for u in COHORT for n in REQUIRED[u]};sdkImage=image(lib.read_bytes(),'iphone',required,6);sdkMap=linkmap(mapping.read_bytes(),sdkImage,owners)
        stable();headers_stable();tools_stable();status='passed'
    except BaseException as exc:
        failureType=type(exc).__name__;failureDetail=(str(exc)+' | '+','.join(str(r.lineno)for r in traceback.extract_tb(exc.__traceback__)if Path(r.filename).resolve()==Path(__file__).resolve()))[:1100]
    finally:
        finalInputFailure=None
        try:
            if closure:stable()
            tools_stable();headers_stable()
        except BaseException as exc:
            finalInputFailure=dict(failureType=type(exc).__name__,failureDetail=str(exc)[:1100]);status='failed'
        toolAfter,toolIdentityErrors=identity_after(toolPaths)
        dependencyAfter,dependencyIdentityErrors=identity_after({n:Path(n)for n in selectedDeps})
        if toolIdentityErrors or dependencyIdentityErrors:status='failed'
        metadata.update(actualHostSystem=sys.platform,actualHostMachine=platform.machine(),sourceFiles=len(closure),productionStrongPolicy=66,cohortStrongPolicy=72,productionVisiblePolicy=47,productionHiddenPolicy=19,macObjects=[objectRows[str(p)]for p in objects.values()if p.parent.name=='mac'],macImages=macImages,primary=sdkRows['primary'],repeat=sdkRows['repeat'],allTwelvePairsByteexact=True if status=='passed'else None,archive=archiveRow,SDKWholeImage=sdkImage,SDKWholeMap=sdkMap,mutants=mutants,executionInputIdentitiesBefore=toolBefore,executionInputIdentitiesAfter=toolAfter,executionInputIdentityErrors=toolIdentityErrors,compileDependencyIdentitiesBefore=selectedDeps,compileDependencyIdentitiesAfter=dependencyAfter,compileDependencyIdentityErrors=dependencyIdentityErrors,SDKHeaderContentsRetained=False,rawFrameDependencyAndMacCodeRetained=True if status=='passed'else None,instructionRelocationsIndependentlyReplayed=False,partial=status!='passed')
        for name,data in [('commands.json',commands),('frames.json',frames),('metadata.json',metadata),('source-identities.json',closure)]:save(out/name,data)
        result=dict(schema='own-real-host-FLS-INC-tag-join-v1',status=status,failedGate=None if status=='passed'else gate,failureType=failureType,failureDetail=failureDetail,finalInputFailure=finalInputFailure,artifactCollectionErrors=[],completed=completed,retirementUncertain=retirementUncertain,toolDescendantDrainProved=False,actualMacRuntime=True if status=='passed'else None,SDKCompileLink=True if status=='passed'else False,syntheticCallbacksOnly=True,sourceRawFLSG2OrRecordOrGameAdmission=False,physicalIPhoneRuntime=False)
        save(out/'result.json',result)
        # Only owned, explicitly retained paths are eligible. Collection errors
        # preserve five reports but prohibit the outer wrapper accepting prefix.
        paths=[out/n for n in('commands.json','frames.json','metadata.json','result.json','source-identities.json')]
        paths+=list((out/'logs').iterdir());paths+=retained
        selected,collectionErrors=artifact_closure(out,paths)
        if collectionErrors:
            status='failed';result.update(status='failed',artifactCollectionErrors=collectionErrors,actualMacRuntime=None,SDKCompileLink=False)
            save(out/'result.json',result);selected['result.json']=pin(out/'result.json')
        save(out/'artifact-index.json',dict(schema='own-authority-join-exact-output-closure-v1',files=selected,retirementUncertain=retirementUncertain,status=status,collectionErrors=collectionErrors))
    return 0 if status=='passed'else 1

if __name__=='__main__':raise SystemExit(main())
