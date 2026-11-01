"""Hash-bound own finite PCM SDK compile and native Mac synthetic tests."""
from pathlib import Path
import hashlib,json,os,platform,shutil,subprocess,sys
from inspect_macho import inspect
H=Path(__file__).resolve().parent
def ident(p):
 b=Path(p).read_bytes();return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def main():
 assert platform.system()=='Darwin'and platform.machine()=='arm64'
 assert os.environ['GITHUB_REPOSITORY']=='lasmith1689-sys/iphone-native-game-ports-public'
 assert os.environ['GITHUB_REF']=='refs/heads/codex/native-pcm-resampler-sdk-v1-20261031'
 assert os.environ['GITHUB_EVENT_NAME']=='push'
 manifest=json.loads((H/'source-manifest.json').read_bytes())
 assert set(manifest)=={'format','files','generatedOracle'}and manifest['format']=='NGP-own-finite-PCM-SDK-source-v1'
 assert set(manifest['files'])=={'ngp_pcm_resampler_v1.c','ngp_pcm_resampler_v1.h','test_native.c','test_apple.c','make_vectors.py','run_apple.py','inspect_macho.py','test_macho.py'}
 for n,w in manifest['files'].items():assert not(H/n).is_symlink()and ident(H/n)==w
 O=Path(sys.argv[1]).resolve();assert not O.exists()and not any(p.is_symlink()for p in(O,*O.parents));O.mkdir(parents=True)
 src=O/'source';src.mkdir()
 for n in manifest['files']:shutil.copyfile(H/n,src/n)
 commands=[];environment={k:v for k,v in os.environ.items()if k in('PATH','HOME','TMPDIR','DEVELOPER_DIR')}
 def run(label,args,timeout=90):
  # Exact reviewed finite tools/tests; shell is never used. No credentials in env.
  p=subprocess.run(list(map(str,args)),cwd=O,env=environment,stdin=subprocess.DEVNULL,capture_output=True,timeout=timeout)
  assert len(p.stdout)+len(p.stderr)<=1048576
  (O/(label+'.stdout.txt')).write_bytes(p.stdout);(O/(label+'.stderr.txt')).write_bytes(p.stderr)
  commands.append(dict(label=label,arguments=list(map(str,args)),cwd=str(O),exitCode=p.returncode,stdout=ident(O/(label+'.stdout.txt')),stderr=ident(O/(label+'.stderr.txt'))))
  (O/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  print(label,p.returncode,flush=True);assert p.returncode==0,label
  return p.stdout.decode('utf-8').strip()
 run('reader-controls',[sys.executable,'-B',src/'test_macho.py'])
 run('xcode-version',['xcodebuild','-version']);run('oracle',[sys.executable,'-B',src/'make_vectors.py'])
 assert ident(src/'vectors.inc')==manifest['generatedOracle']
 sdks={k:run(k+'-sdk',['xcrun','--sdk',k,'--show-sdk-path'])for k in('iphoneos','macosx')}
 version=run('ios-sdk-version',['xcrun','--sdk','iphoneos','--show-sdk-version']);assert int(version.split('.')[0])>=26
 clang=run('clang-path',['xcrun','--sdk','iphoneos','--find','clang']);assert Path(clang).is_file()
 run('clang-version',[clang,'--version']);tool_identity=ident(clang)
 flags=['-std=c11','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-Wall','-Wextra','-Werror','-fstack-usage']
 for name in('primary','repeat'):
  run('ios-'+name,[clang,'-target','arm64-apple-ios26.0','-isysroot',sdks['iphoneos'],*flags,'-c',src/'ngp_pcm_resampler_v1.c','-o',O/(name+'.o')])
 assert ident(O/'primary.o')==ident(O/'repeat.o')
 library=O/'libNgpPCMResamplerV1.dylib'
 run('ios-strict-link',[clang,'-target','arm64-apple-ios26.0','-isysroot',sdks['iphoneos'],'-dynamiclib','-Wl,-undefined,error','-Wl,-no_uuid','-Wl,-no_adhoc_codesign','-Wl,-no_fixup_chains','-Wl,-install_name,@rpath/libNgpPCMResamplerV1.dylib',O/'primary.o','-o',library])
 run('ios-load-commands',['xcrun','otool','-l',library]);run('ios-symbols',['xcrun','nm','-m',library])
 native_facts=inspect(library.read_bytes())
 executable=O/'pcm-native-tests'
 run('mac-native-build',[clang,'-target','arm64-apple-macos14.0','-isysroot',sdks['macosx'],*flags,src/'ngp_pcm_resampler_v1.c',src/'test_apple.c','-o',executable])
 result=json.loads(run('mac-native-test',[executable],30))
 assert result==dict(format='NGP-PCM-native-Mac-test-v1',testReturn=0,vectors=108,wholeAndPartitionedRuns=216,AppleAudioExecuted=False,gameExecuted=False)
 assert ident(clang)==tool_identity
 for n,w in manifest['files'].items():assert ident(H/n)==w==ident(src/n)
 assert ident(src/'vectors.inc')==manifest['generatedOracle']
 report=dict(format='NGP-own-PCM-actual-SDK-result-v1',head=os.environ['GITHUB_SHA'],sourceManifest=ident(H/'source-manifest.json'),
  sourceFiles=manifest['files'],generatedOracle=manifest['generatedOracle'],compiler=dict(path=clang,identity=tool_identity),sdkVersion=version,
  primaryObject=ident(O/'primary.o'),repeatObject=ident(O/'repeat.o'),strictLinkedLibrary=ident(library),nativeFacts=native_facts,nativeMacTest=result,
  iPhoneOrAppleAudioOrGameExecuted=False,SDKObjectsAndLibraryAreUnsigned=True,signingOrCredentialsUsed=False)
 (O/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
