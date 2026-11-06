"""Exact trusted driver primitives, no main/native action on import."""
from pathlib import Path
import hashlib,json,os,platform,shlex,stat,sys
from bounded_process import read,signature
BASE=('-std=c11','-O0','-g0','-UNDEBUG','-fno-ident','-fno-inline','-fno-lto','-fno-optimize-sibling-calls','-ffp-contract=off','-fno-fast-math','-fno-strict-aliasing','-fstack-usage','-MD','-ferror-limit=20')
REV='f40cf03f80cdb7e741d43e53b7e706e8c1394bcf'
VENDOR={'miniaudio.h':(4099492,'7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6'),'LICENSE':(2597,'457f1b500e0adf6bc059edddfa78a2f62012e7c3bb43476c20e0bd23b25ba0eb')}
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
