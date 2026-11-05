"""Bounded genuine SDK build and Mac-only tests of self-authored support."""
import argparse,hashlib,json,os,platform,re,shlex,struct,subprocess,sys
from pathlib import Path

SOURCES=('div64.c','div64.h','monotonic.c','monotonic.h','provider_darwin.c',
         'provider_windows.c','test_native.c','run_sdk.py','README.md')
PRODUCTION=('div64.c','monotonic.c','provider_darwin.c')
EXPECTED={'_pc64_udiv64_bits','_pc64_sdiv64_bits','_pc_clock_scale',
          '_pc_clock_read','_pc_clock_platform'}
def identity(p):
    return {'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
def dependency_allowed(path,source,sdk,resource):
    path=path.resolve(strict=True)
    roots=(p.resolve(strict=True) for p in (source,sdk,resource))
    return any(path.is_relative_to(root) for root in roots)
def archive_catalogue(text):
    members=text.splitlines()
    assert members==['__.SYMDEF SORTED',*[n+'.o' for n in PRODUCTION]],members
    return members[1:]
def inspect_ios(raw):
    assert len(raw)>=32
    magic,cpu,subcpu,kind,count,size,flags,reserved=struct.unpack_from('<8I',raw)
    assert (magic,cpu,kind)==(0xfeedfacf,0x100000c,1)
    assert count<100 and size<=len(raw)-32
    cursor=32;version=None;text_size=0
    for unused in range(count):
        command,length=struct.unpack_from('<2I',raw,cursor)
        assert length>=8 and cursor+length<=32+size
        if command==0x32:
            assert version is None and length>=24
            version=struct.unpack_from('<4I',raw,cursor+8)
        if command==0x19:
            assert length>=72
            sections=struct.unpack_from('<I',raw,cursor+64)[0]
            assert length==72+sections*80
            for i in range(sections):
                at=cursor+72+i*80
                name=raw[at:at+16].split(b'\0',1)[0]
                n=struct.unpack_from('<Q',raw,at+40)[0]
                offset=struct.unpack_from('<I',raw,at+48)[0]
                if name==b'__text':
                    assert n>0 and offset+n<=len(raw);text_size+=n
        cursor+=length
    assert cursor==32+size and version and version[0]==2 and version[1]==26<<16
    assert version[2]>=27<<16 and text_size>0
    return {'cpu':'arm64','platform':version[0],'minimumPacked':version[1],
            'SDKPacked':version[2],'textBytes':text_size}
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    assert sys.platform=='darwin' and platform.machine()=='arm64','native Mac ARM64 host required'
    source=Path(__file__).resolve().parent;out=a.output.resolve()
    assert not out.exists() and not source.is_symlink()
    out.mkdir(parents=True);(out/'logs').mkdir();(out/'primary').mkdir();(out/'repeat').mkdir()
    closure={n:identity(source/n) for n in SOURCES}
    (out/'source-identities.json').write_text(json.dumps(closure,sort_keys=True,indent=2)+'\n')
    commands=[]
    def run(label,args,timeout=120):
        assert re.fullmatch('[a-z0-9-]+',label)
        command=[str(x) for x in args]
        p=subprocess.run(command,cwd=out,capture_output=True,timeout=timeout,
                         env={**os.environ,'ZERO_AR_DATE':'1','LC_ALL':'C'})
        (out/'logs'/f'{label}.stdout').write_bytes(p.stdout)
        (out/'logs'/f'{label}.stderr').write_bytes(p.stderr)
        commands.append({'label':label,'command':command,'exitCode':p.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        assert p.returncode==0,(label,p.returncode,p.stderr.decode(errors='replace')[-3000:])
        return p.stdout.decode()
    clang=run('clang-path',['xcrun','--find','clang']).strip()
    nm=run('nm-path',['xcrun','--find','nm']).strip()
    ar=run('ar-path',['xcrun','--find','ar']).strip()
    sdk=Path(run('iphone-sdk-path',['xcrun','--sdk','iphoneos','--show-sdk-path']).strip()).resolve(strict=True)
    resource=Path(run('compiler-resource-path',[clang,'-print-resource-dir']).strip()).resolve(strict=True)
    sdk_version=run('iphone-sdk-version',['xcrun','--sdk','iphoneos','--show-sdk-version']).strip()
    assert re.fullmatch(r'27\.\d+(?:\.\d+)?',sdk_version) and sdk.is_dir()
    run('compiler-version',[clang,'--version'])
    run('xcode-version',['xcodebuild','-version'])
    flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-fno-stack-protector',
           '-fvisibility=hidden','-fno-builtin','-I',str(source)]
    artifacts={};definitions=set();undefined=set();deps={}
    for cohort in ('primary','repeat'):
        for i,name in enumerate(PRODUCTION):
            obj=out/cohort/(name+'.o');dep=obj.with_suffix('.d')
            run(f'ios-{cohort}-{i}',[clang,'-target','arm64-apple-ios26.0','-isysroot',sdk,
                *flags,'-ffreestanding','-fstack-usage','-MD','-MF',dep,
                '-c',source/name,'-o',obj])
            metadata=inspect_ios(obj.read_bytes())
            if cohort=='repeat':
                assert obj.read_bytes()==(out/'primary'/obj.name).read_bytes(),name
                assert obj.with_suffix('.su').read_bytes()==(out/'primary'/obj.with_suffix('.su').name).read_bytes()
            else:
                artifacts[name]={'identity':identity(obj),'metadata':metadata,
                                 'stackUsage':identity(obj.with_suffix('.su'))}
                syms=run(f'symbols-{i}',[nm,'-g',obj])
                for line in syms.splitlines():
                    parts=line.split()
                    assert len(parts) in (2,3),line
                    kind,symbol=parts[-2:]
                    if kind=='U':undefined.add(symbol)
                    else:
                        assert kind=='T' and symbol in EXPECTED and symbol not in definitions,line
                        definitions.add(symbol)
                deptext=dep.read_text().replace('\\\n',' ')
                paths=[Path(x).resolve(strict=True) for x in shlex.split(deptext.split(':',1)[1])]
                assert source/name in paths
                assert any(p.name=='mach_time.h' and p.is_relative_to(sdk) for p in paths) if name=='provider_darwin.c' else True
                for p in paths:
                    assert dependency_allowed(p,source,sdk,resource),str(p)
                    deps[str(p)]=identity(p)
    assert definitions==EXPECTED
    external=undefined-definitions
    assert {'_mach_absolute_time','_mach_timebase_info'}<=external
    assert external<={'_mach_absolute_time','_mach_timebase_info','_memcpy'},external
    archive=out/'native-monotonic-ios-arm64.a'
    run('archive',[ar,'rcs',archive,*[out/'primary'/(n+'.o') for n in PRODUCTION]])
    members=archive_catalogue(run('archive-members',[ar,'t',archive]))
    native={}
    for mode in ('provider-failures','real-clock'):
        executable=out/f'mac-{mode}'
        sources=[source/'test_native.c',source/'monotonic.c',source/'div64.c']
        extra=['-DPC_CLOCK_TEST_PROVIDER'] if mode=='provider-failures' else []
        if not extra:sources.append(source/'provider_darwin.c')
        run(f'mac-compile-{mode}',[clang,'-arch','arm64',*flags,*extra,*sources,'-o',executable])
        stdout=run(f'mac-run-{mode}',[executable],60)
        rows=[json.loads(x) for x in stdout.splitlines()]
        assert rows[-1]['failures']==0 and rows[-1]['scaleVectors']==65914
        if mode=='real-clock':
            assert rows[0]['domain']==2 and rows[0]['nativeSamples']==4096
            assert rows[0]['elapsedNanoseconds']>0
        native[mode]={'executable':identity(executable),'outcomes':rows}
    assert {n:identity(source/n) for n in SOURCES}==closure
    (out/'dependencies.json').write_text(json.dumps(deps,sort_keys=True,indent=2)+'\n')
    result={'status':'native-support-sdk-and-host-tests-passed','AppleSDKUsed':True,
        'SDKVersion':sdk_version,'objects':artifacts,'definitions':sorted(definitions),
        'externalSymbols':sorted(external),'repeatedObjectsIdentical':True,
        'archive':identity(archive),'nativeMacTests':native,'iPhoneExecuted':False,
        'gameCodeIncluded':False,'gameExecuted':False,'signed':False,'installable':False}
    (out/'result.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('status','SDKVersion','repeatedObjectsIdentical','iPhoneExecuted')}))
if __name__=='__main__':main()
