"""Owned PCM core: actual Apple headers/objects and synthetic Darwin mixer tests."""
from pathlib import Path
import hashlib,json,os,platform,subprocess,time,zipfile

ROOT=Path.cwd(); WORK=ROOT/'sdk-work'; OUT=ROOT/'sdk-result'; COMMANDS=[]
BRANCH='codex/owned-pcm-sdk-v1-20261107'
UNITS=['sample_control','sample_playback','stereo_gain','ngp_voice_service_v1','ngp_voice_tokens_v1','ngp_positional_provider_v1','graph']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(label,args,timeout=240):
    argv=list(map(str,args)); start=time.monotonic()
    p=subprocess.run(argv,cwd=WORK,capture_output=True,timeout=timeout)
    (OUT/(label+'.stdout')).write_bytes(p.stdout);(OUT/(label+'.stderr')).write_bytes(p.stderr)
    COMMANDS.append(dict(label=label,argv=argv,exitCode=p.returncode,seconds=time.monotonic()-start))
    (OUT/'commands.json').write_text(json.dumps(COMMANDS,indent=2)+'\n')
    assert p.returncode==0,label
    return p.stdout.decode().strip()
def main():
    assert platform.system()=='Darwin' and platform.machine()=='arm64'
    assert os.environ['GITHUB_REPOSITORY']=='lasmith1689-sys/iphone-native-game-ports-public'
    assert os.environ['GITHUB_REF']=='refs/heads/'+BRANCH and os.environ['GITHUB_EVENT_NAME']=='push'
    WORK.mkdir();OUT.mkdir()
    manifest=json.loads((ROOT/'pcm-source.json').read_bytes());archive=ROOT/'pcm-source.zip'
    assert sha(archive)==manifest['archiveSha256']
    with zipfile.ZipFile(archive) as z:
        assert len(z.infolist())==len(manifest['files']) and set(z.namelist())==set(manifest['files'])
        for i in z.infolist():
            n=i.filename
            assert not i.is_dir() and not n.startswith('/') and '..' not in n.split('/') and '\\' not in n
            assert (i.external_attr>>16)&0o170000!=0o120000
            b=z.read(i);assert hashlib.sha256(b).hexdigest()==manifest['files'][n]
            p=WORK/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b)
    clang=run('clang-path',['xcrun','--find','clang'])
    sdk=run('iphone-sdk-path',['xcrun','--sdk','iphoneos','--show-sdk-path'])
    version=run('iphone-sdk-version',['xcrun','--sdk','iphoneos','--show-sdk-version'])
    assert version=='27.1','Exact iPhone SDK27.1 cohort required'
    run('xcode-version',['xcodebuild','-version']);run('clang-version',[clang,'--version'])
    base=['-std=c17','-O2','-g0','-fno-fast-math','-ffp-contract=off','-fno-strict-aliasing',
          '-fstack-protector','-fvisibility=hidden','-fstack-usage','-Wall','-Wextra',
          '-I'+str(WORK/'source'),'-I'+str(WORK/'vendor')]
    rows={}; stacks={}
    for phase in ('primary','repeat'):
        dest=OUT/phase;dest.mkdir();objects={};reports={}
        for unit in UNITS:
            obj=dest/(unit+'.o')
            strict=[] if unit=='graph' else ['-Werror']
            run(phase+'-'+unit,[clang,'-target','arm64-apple-ios26.0','-isysroot',sdk,*base,*strict,
                '-Xclang','-fdump-record-layouts','-MD','-MF',dest/(unit+'.d'),'-c',WORK/'source'/(unit+'.c'),'-o',obj])
            su=obj.with_suffix('.su'); assert su.exists() and su.stat().st_size>0
            lines=su.read_text().splitlines(); assert all(x.split('\t')[-1]=='static' for x in lines),unit
            objects[obj.name]=sha(obj);reports[su.name]=dict(sha256=sha(su),rows=lines)
            run(phase+'-'+unit+'-undefined',['xcrun','nm','-u',obj])
        rows[phase]=objects;stacks[phase]=reports
    assert rows['primary']==rows['repeat'] and stacks['primary']==stacks['repeat']
    (OUT/'stack-reports.json').write_text(json.dumps(stacks,indent=2)+'\n')
    mac=run('mac-sdk-path',['xcrun','--sdk','macosx','--show-sdk-path'])
    mac_version=run('mac-sdk-version',['xcrun','--sdk','macosx','--show-sdk-version'])
    dest=OUT/'darwin';dest.mkdir();objects=[]
    for unit in UNITS:
        obj=dest/(unit+'.o');strict=[] if unit=='graph' else ['-Werror']
        run('darwin-'+unit,[clang,'-arch','arm64','-isysroot',mac,*base,*strict,'-c',WORK/'source'/(unit+'.c'),'-o',obj])
        objects.append(obj)
    results={}
    for fixture in ('test_playback','test_gain'):
        exe=dest/fixture
        run(fixture+'-link',[clang,'-arch','arm64','-isysroot',mac,*base,'-Werror',WORK/'source'/(fixture+'.c'),*objects,'-pthread','-o',exe])
        results[fixture]=json.loads(run(fixture+'-run',[exe]))
    p=results['test_playback'];g=results['test_gain']
    assert p['formats']==4 and p['refusals']==25 and p['nonzeroPcmSamples']>0 and p['phone'] is False
    assert all(p[k] is True for k in ('copiedSource','ratePitchObserved','loopResumeObserved','noOldReplayAfterInit'))
    assert g['cases']==49 and g['refusals']==9 and len(g['observations'])==49 and g['maxPcmError']<.00001
    assert g['callerPcmCleared'] is True and g['actualVoiceGraph'] is True and g['phone'] is False
    result=dict(passed=True,commit=os.environ['GITHUB_SHA'],iphoneSDK=version,macSDK=mac_version,
                sourceManifestSha256=sha(ROOT/'pcm-source.json'),objects=rows,syntheticDarwin=results,
                actualDevicePlayback=False,guestOwnership=False,productionStackCatalog=False,
                limits=['Seven coherent units; unused guest-call adapter intentionally excluded.',
                        'Actual macOS synthetic PCM only; no original mixer parity or game runtime.',
                        'Per-function SDK frames are not whole-stack admission or a cache lease.'])
    (OUT/'result.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
