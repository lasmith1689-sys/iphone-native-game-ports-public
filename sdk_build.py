"""Build public, self-authored GL support against a real Apple SDK.

No retail assets, game engine code, secrets, signing, phone access or app output.
"""
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time
import zipfile

ROOT=Path.cwd()
WORK=ROOT/'sdk-work'
RESULT=ROOT/'sdk-result'
PIN='2037865590d410bcd7b8d1f22b841472ed6d39ac'
COMMANDS=[]

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def run(label,args,cwd=None,timeout=300):
    argv=list(map(str,args));started=time.monotonic()
    p=subprocess.run(argv,cwd=cwd or WORK,capture_output=True,timeout=timeout)
    (RESULT/(label+'.stdout')).write_bytes(p.stdout)
    (RESULT/(label+'.stderr')).write_bytes(p.stderr)
    COMMANDS.append(dict(label=label,argv=argv,cwd=str(cwd or WORK),exitCode=p.returncode,
                         seconds=time.monotonic()-started))
    (RESULT/'commands.json').write_text(json.dumps(COMMANDS,indent=2)+'\n')
    if p.returncode:raise RuntimeError(f'{label} exited {p.returncode}; see retained logs')
    return p.stdout.decode().strip()

def main():
    assert os.environ['GITHUB_REPOSITORY']=='lasmith1689-sys/iphone-native-game-ports-public'
    assert os.environ['GITHUB_REF']=='refs/heads/codex/native-menu-gl-pending-drain-sdk-v1-20261107'
    assert os.environ['GITHUB_EVENT_NAME']=='push'
    WORK.mkdir();RESULT.mkdir()
    manifest=json.loads((ROOT/'menu-gl-source.json').read_bytes())
    package=ROOT/'menu-gl-source.zip'
    assert digest(package)==manifest['archiveSha256']
    with zipfile.ZipFile(package) as archive:
        assert len(archive.infolist())==len(manifest['files'])==21
        assert set(archive.namelist())==set(manifest['files'])
        for info in archive.infolist():
            name=info.filename
            assert not name.startswith('/') and '..' not in name.split('/') and '\\' not in name
            assert not info.is_dir() and not ((info.external_attr>>16)&0o170000)==0o120000
            raw=archive.read(info)
            assert hashlib.sha256(raw).hexdigest()==manifest['files'][name]
            out=WORK/name;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(raw)
    clang=run('clang-path',['xcrun','--find','clang'])
    sdk=run('sdk-path',['xcrun','--sdk','iphoneos','--show-sdk-path'])
    sdk_version=run('sdk-version',['xcrun','--sdk','iphoneos','--show-sdk-version'])
    assert sdk_version=='27.1', 'exact device SDK27.1 cohort required'
    run('xcode-version',['xcodebuild','-version'])
    source=WORK/'backend'
    run('clone-backend',['git','-c','credential.helper=','-c','core.hooksPath=/dev/null','clone','--no-checkout','https://github.com/ptitSeb/gl4es.git',source])
    run('checkout-backend',['git','-C',source,'checkout','--detach',PIN])
    assert run('backend-head',['git','-C',source,'rev-parse','HEAD'])==PIN
    for name,value in manifest['backendPins'].items():assert digest(source/name)==value,name
    aliaslines=[line.strip() for p in (source/'src').rglob('*.c') for line in p.read_text().splitlines() if 'AliasDecl(' in line]
    assert aliaslines==['AliasDecl(void,gl4es_glEnableClientStatei,(GLenum array, GLuint index),gl4es_glEnableClientStateIndexed);',
                        'AliasDecl(void,gl4es_glDisableClientStatei,(GLenum array, GLuint index),gl4es_glDisableClientStateIndexed);']
    build=WORK/'backend-build';adapter=WORK/'apple_alias_adapter.h'
    run('configure',['cmake','-S',source,'-B',build,'-G','Unix Makefiles',
        '-DCMAKE_SYSTEM_NAME=iOS','-DCMAKE_SYSTEM_PROCESSOR=arm64','-DCMAKE_OSX_ARCHITECTURES=arm64',
        '-DCMAKE_OSX_SYSROOT='+sdk,'-DCMAKE_OSX_DEPLOYMENT_TARGET=26.0','-DCMAKE_C_COMPILER='+clang,
        '-DCMAKE_C_COMPILER_TARGET=arm64-apple-ios26.0','-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',
        '-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER','-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_C_FLAGS=-fstack-usage -include '+str(adapter),'-DCMAKE_C_FLAGS_RELEASE=-O2 -DNDEBUG',
        '-DNOX11=ON','-DNOEGL=ON','-DSTATICLIB=ON','-DNO_LOADER=ON','-DNO_INIT_CONSTRUCTOR=ON',
        '-DDEFAULT_ES=2','-DGBM=OFF','-DEGL_WRAPPER=OFF','-DUSE_CCACHE=OFF','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
    run('backend-build',['cmake','--build',build,'--target','GL','--parallel','3'],timeout=1200)
    flags=['-target','arm64-apple-ios26.0','-isysroot',sdk,'-O2','-fvisibility=hidden','-fstack-usage',
           '-Wall','-Wextra','-Werror','-Wno-error=deprecated-declarations','-I'+str(source/'include'),
           '-I'+str(WORK/'v13'),'-I'+str(WORK/'composition')]
    sources={name:'v13/'+name+'.c' for name in ('cod_gl_dispatch','cod_gl4es_execute','cod_gl_compressed_stage','cod_gl_owned_profile')}
    sources.update(ngp_dxt_v1='compressed-dxt-v1/ngp_dxt_v1.c',cod_gl_composition_worker='composition/worker/cod_gl_ios.m',
                   cod_gl_host_capsule_v1='composition/cod_gl_host_capsule_v1.m')
    db=json.loads((build/'compile_commands.json').read_bytes())
    shutil.copy2(build/'compile_commands.json',RESULT/'backend-compile-commands.json')
    shutil.copy2(source/'lib/libGL.a',RESULT/'libGL.a')
    for stack_file in build.rglob('*.su'):
        out=RESULT/'backend-stack'/stack_file.relative_to(build)
        out.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(stack_file,out)
    row=next(row for row in db if Path(row['file']).resolve()==(source/'src/gl/glstate.c').resolve())
    original=row.get('arguments') or shlex.split(row['command'])
    results={};stack_results={}
    for phase in ('primary','repeat'):
        out=RESULT/phase;out.mkdir()
        objects=[]
        for name,relative in sources.items():
            obj=out/(name+'.o')
            language=['-fobjc-arc','-fblocks'] if relative.endswith('.m') else ['-std=c11','-fno-fast-math']
            run(phase+'-'+name,[clang,*flags,*language,'-Xclang','-fdump-record-layouts','-MMD','-MF',out/(name+'.d'),'-c',WORK/relative,'-o',obj])
            objects.append(obj)
        matching=list(original)
        matching[matching.index('-c')+1]=str(WORK/'v13/cod_gl4es_unpack.c')
        matching[matching.index('-o')+1]=str(out/'cod_gl4es_unpack.o')
        matching+=['-I'+str(source/'src/gl'),'-MMD','-MF',str(out/'cod_gl4es_unpack.d')]
        run(phase+'-matching-private-layout',matching,cwd=row['directory'])
        objects.append(out/'cod_gl4es_unpack.o')
        run(phase+'-link',[clang,'-target','arm64-apple-ios26.0','-isysroot',sdk,'-dynamiclib',*objects,
            '-Wl,-force_load,'+str(source/'lib/libGL.a'),'-framework','UIKit','-framework','QuartzCore',
            '-framework','CoreGraphics','-framework','OpenGLES','-framework','Foundation','-framework','CoreFoundation',
            '-Wl,-no_fixup_chains','-Wl,-no_adhoc_codesign','-install_name','@rpath/libCodNativeGLCompositionV1.dylib',
            '-o',out/'libCodNativeGLCompositionV1.dylib'])
        results[phase]={p.name:digest(p) for p in objects+[out/'libCodNativeGLCompositionV1.dylib']}
        stack_results[phase]={}
        for obj in objects:
            stack=obj.with_suffix('.su');raw=stack.read_bytes();rows=raw.decode().splitlines()
            assert rows and all(line.rsplit('\t',2)[1].isdigit() and line.rsplit('\t',2)[2]=='static' for line in rows), obj.name
            stack_results[phase][stack.name]=dict(sha256=digest(stack),bytes=len(raw),rows=rows)
    assert results['primary']==results['repeat']
    assert stack_results['primary']==stack_results['repeat']
    (RESULT/'support-stack.json').write_text(json.dumps(stack_results,indent=2)+'\n')
    # Run the same synthetic bridge boundary tests on Darwin, separate from GL.
    test_sources=[WORK/p for n,p in sources.items() if p.endswith('.c') and n!='cod_gl4es_execute']
    test=RESULT/'boundary-tests.dylib'
    mac_sdk=run('mac-sdk-path',['xcrun','--sdk','macosx','--show-sdk-path'])
    run('darwin-tests-compile',[clang,'-target','arm64-apple-macos15.0','-isysroot',mac_sdk,'-std=c11','-O2','-fno-fast-math','-fno-builtin','-fvisibility=hidden',
        '-Wall','-Wextra','-Werror','-I',WORK/'v13','-dynamiclib',*test_sources,WORK/'texture_fixture.c',WORK/'menu_fixture.c',WORK/'drain_fixture.c','-o',test])
    lib=ctypes.CDLL(str(test));texture=lib.texture_fixture
    texture.argtypes=(ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_int);texture.restype=ctypes.c_int
    for f in (0x8050,0x8056,0x1907,0x1908):
        for w in (1,2,4,8,16):
            for a in (1,2,4,8):
                for fail in (0,1):assert not texture(f,w,a,fail),(f,w,a,fail)
    lib.menu_fixture.restype=ctypes.c_int
    assert not lib.menu_fixture()
    cinematic=lib.cinematic_fixture
    cinematic.argtypes=(ctypes.c_uint32,ctypes.c_uint32,ctypes.c_int)
    cinematic.restype=ctypes.c_int
    for width in (256,512):
        for alignment in (1,2,4,8):
            for fail in (0,1):assert not cinematic(width,alignment,fail),(width,alignment,fail)
    drain=lib.drain_fixture
    drain.argtypes=(ctypes.c_uint32,);drain.restype=ctypes.c_int
    for case in range(36):assert not (result:=drain(case)), ('drain',case,result)
    result=dict(drainControls=36,repeatedStackReportsExact=True,stackReports=stack_results,status='PASS',abi=13,sourceManifest=manifest,sdkVersion=sdk_version,objects=results,
                repeatedObjectsAndLibraryExact=True,textureCases=160,cinematicCases=16,functionCount=57,menuFixturePassed=True,
                realGLExecuted=False,gameHostCompiled=False,appCreated=False,signed=False,phoneContacted=False)
    (RESULT/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('sourceManifest','objects','stackReports')}))

if __name__=='__main__':main()
