"""Own controller object compile and own host wait tests; never execute a game."""
from pathlib import Path
import hashlib,json,os,platform,re,sys,time
from bounded_process import execute,identity,read,need

HOME=Path(__file__).resolve().parent
def save(path,value):
    path.write_text(json.dumps(value,sort_keys=True,indent=2)+'\n',encoding='utf-8')
def tool_identity(path):
    p=Path(path).resolve(strict=True);before=p.stat()
    need(p.is_file() and before.st_size<=1024**3,'bounded selected compiler')
    digest=hashlib.sha256()
    with p.open('rb') as f:
        for chunk in iter(lambda:f.read(65536),b''):digest.update(chunk)
    after=p.stat()
    need((before.st_dev,before.st_ino,before.st_size,before.st_mtime_ns)==
         (after.st_dev,after.st_ino,after.st_size,after.st_mtime_ns),'stable compiler')
    return {'bytes':before.st_size,'sha256':digest.hexdigest()}
def run(output):
    out=Path(output).resolve()
    need(not out.exists() and not out.is_relative_to(HOME),'fresh separate SDK output')
    out.mkdir(mode=0o700);work=out/'work';work.mkdir()
    manifest=read(HOME/'payload-manifest.json',65536);files=json.loads(manifest)['files']
    rows=[];deadline=time.monotonic()+480
    result={'status':'failed','controllerObjectCompile':False,'hostPolicyTests':False,
            'engineLinked':False,'appPackaged':False,'installable':False,
            'gameExecuted':False,'physicalIPhoneTest':False,'guardMetadataAdmitted':False,
            'recoveredCodeIncluded':False,'retailAssetsIncluded':False,
            'manifest':identity(manifest)}
    def stable():
        actual={p.relative_to(HOME).as_posix() for p in HOME.rglob('*') if p.is_file()}
        need(actual==set(files)|{'payload-manifest.json'},'exact own payload membership')
        need(read(HOME/'payload-manifest.json',65536)==manifest,'manifest stable')
        for name,wanted in files.items():
            p=HOME/name;need(p.resolve().is_relative_to(HOME),'own payload path')
            need(identity(read(p,262144))==wanted,'own source identity: '+name)
    env={k:v for k,v in os.environ.items() if k in ('PATH','HOME','TMPDIR','TMP','TEMP')}
    env.update(PYTHONDONTWRITEBYTECODE='1',PYTHONNOUSERSITE='1',LC_ALL='C')
    def command(label,argv):
        stable();row,bodies=execute(label,argv,work,env,timeout=60,cap=262144,total_deadline=deadline)
        rows.append(row);save(out/'commands.json',rows)
        need(row['driverReaped'] and row['processGroupAbsentObserved'] and not row['timedOut'] and not row['streamExceeded'],'bounded retired tool: '+label)
        stable();need(row['exitCode']==0,'command failed: '+label)
        return bodies['stdout'].decode('utf-8','strict').strip()
    try:
        stable();need(sys.platform=='darwin' and platform.machine()=='arm64','actual ARM64 macOS SDK host')
        os.chdir(HOME)
        sdk=command('sdk-path',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-path'])
        version=command('sdk-version',['/usr/bin/xcrun','--sdk','iphoneos','--show-sdk-version'])
        need(re.fullmatch(r'27\.1(?:\.\d+)?',version) is not None,'selected real iPhoneOS27.1 SDK')
        clang=command('clang-path',['/usr/bin/xcrun','--sdk','iphoneos','--find','clang'])
        need(Path(sdk).is_dir() and Path(clang).is_file(),'actual SDK/compiler paths')
        result.update(sdkVersion=version,sdkPath=sdk,compiler=clang,
                      compilerIdentity=tool_identity(clang))
        command('compiler-version',[clang,'--version'])
        include=['source','adapter','engine','power','composition','composition/worker','v12b',
                 'support/native/platform/cod-gl-ios/input-v3-bind-v3',
                 'support/native/platform/cod-gl-ios/input-v1']
        flags=['-target','arm64-apple-ios26.0','-isysroot',sdk,'-std=c17','-O2','-g0',
               '-Wall','-Wextra','-Werror','-fno-omit-frame-pointer','-fno-optimize-sibling-calls',
               '-fstack-usage','-fno-common','-fvisibility=hidden','-fno-lto']
        flags += [item for directory in include for item in ('-I',str(HOME/directory))]
        selection=[('controller','source/NGPRetainedGameControllerV1.m',True),
                   ('wait-policy','source/ngp_boot_wait_v1.c',False)]
        objects={}
        for role,source,objc in selection:
            per=flags+(['-fobjc-arc','-fobjc-arc-exceptions','-fexceptions','-fblocks'] if objc else [])
            for suffix in ('','-repeat'):
                stem=role+suffix
                command('compile-'+stem,[clang,*per,'-MMD','-MF',str(work/(stem+'.d')),
                                       '-c',str(HOME/source),'-o',str(work/(stem+'.o'))])
            o=read(work/(role+'.o'),4*1024**2);other=read(work/(role+'-repeat.o'),4*1024**2)
            su=read(work/(role+'.su'),262144);other_su=read(work/(role+'-repeat.su'),262144)
            need(o==other and su==other_su,'repeat object and stack usage exact: '+role)
            need(o[:4]==b'\xcf\xfa\xed\xfe' and int.from_bytes(o[4:8],'little')==0x100000c,'actual ARM64 Mach-O object')
            need(b'dynamic' not in su,'no unaccounted dynamic frame in own object')
            objects[role]={'object':identity(o),'stackUsage':identity(su),'repeatExact':True}
        result['objects']=objects;result['controllerObjectCompile']=True
        # Only this newly authored finite arithmetic test executes on the Mac.
        mac_sdk=command('macos-sdk-path',['/usr/bin/xcrun','--sdk','macosx','--show-sdk-path'])
        need(Path(mac_sdk).is_dir(),'actual separate macOS SDK for own host test')
        result['hostTestMacOSSDK']=mac_sdk
        command('host-policy-compile',[clang,'-target','arm64-apple-macos14.0','-isysroot',mac_sdk,
                                     '-std=c17','-O2','-Wall','-Wextra','-Werror',
                                     '-I',str(HOME/'source'),str(HOME/'tests/test_wait_policy.c'),
                                     str(HOME/'source/ngp_boot_wait_v1.c'),'-o',str(work/'test-wait')])
        host_result=command('host-policy-test',[str(work/'test-wait')]);need(host_result.startswith('PASS wait policy:'),'actual own host test output')
        result['hostPolicyTests']=True
        command('finite-source-contract',[sys.executable,'-B','-m','unittest','discover',
                                         '-s',str(HOME/'tests'),'-p','test_source_contract.py'])
        need(tool_identity(clang)==result['compilerIdentity'],'compiler stable through compile/test')
        stable();result['status']='passed'
    except Exception as exc:
        result['failure']={'type':type(exc).__name__,'reason':str(exc)[:512]}
    finally:
        save(out/'commands.json',rows);save(out/'result.json',result)
        artifacts={}
        for p in sorted(out.rglob('*')):
            if p.is_file() and p.name!='artifacts.json':
                b=read(p,4*1024**2);artifacts[p.relative_to(out).as_posix()]=identity(b)
        save(out/'artifacts.json',artifacts)
    return 0 if result['status']=='passed' else 1
if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('--output',required=True)
    raise SystemExit(run(p.parse_args().output))
