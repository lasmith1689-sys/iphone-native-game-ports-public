"""Bounded own-output evidence projection. No native/tool/network execution."""
from pathlib import Path
import argparse,hashlib,json,os,stat
MAX_FILES=1024;MAX_TOTAL=64*1024**2;MAX_FILE=16*1024**2
def need(v,m):
 if not v:raise ValueError(m)
def key(s):return(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns)
def plan(source,max_total=MAX_TOTAL-1024**2,max_files=MAX_FILES-1):
 need(source.is_dir()and not source.is_symlink(),'regular source tree');root=source.resolve();rows=[];total=0
 paths=sorted(source.rglob('*'));need(len(paths)<MAX_FILES,'path roster with metadata reserve')
 for p in paths:
  need(not p.is_symlink()and p.resolve().is_relative_to(root),'no symlink/escape')
  if p.is_dir():continue
  s=p.lstat();need(stat.S_ISREG(s.st_mode)and s.st_nlink==1 and s.st_size<=MAX_FILE,'regular bounded own output');total+=s.st_size;need(total<=max_total and len(rows)<max_files,'own output aggregate bound');rel=p.relative_to(source);need(len(rel.as_posix())<=512,'relative evidence name bound');rows.append((p,rel,key(s)))
 return rows,total
def collect(source,output):
 need(not output.exists()and not output.is_symlink()and not any(p.is_symlink()for p in output.parents),'fresh retained tree');need(not output.resolve().is_relative_to(source.resolve()),'retention outside source');output.mkdir(parents=True);kept={};complete=False;error=None
 try:
  rows,total=plan(source);need(not output.resolve().is_relative_to(source.resolve()),'retention outside source')
  for p,rel,before in rows:
   q=output/rel;q.parent.mkdir(parents=True,exist_ok=True);h=hashlib.sha256();size=0
   with p.open('rb')as f,q.open('xb')as w:
    need(key(os.fstat(f.fileno()))==before,'stable opened output')
    while True:
     b=f.read(1024**2)
     if not b:break
     size+=len(b);need(size<=before[2],'output grew');h.update(b);w.write(b)
    need(key(os.fstat(f.fileno()))==before and key(p.lstat())==before and size==before[2],'stable copied output')
   kept[rel.as_posix()]=dict(bytes=size,sha256=h.hexdigest())
  complete=True
 except Exception as e:error=dict(errorClass=type(e).__name__,reason=str(e)[:256])
 result=dict(schema='own-build-raw-evidence-retention-v1',allExistingOutputsRetained=complete,files=kept,retainedBytes=sum(x['bytes']for x in kept.values()),maximumBytes=MAX_TOTAL,maximumFiles=MAX_FILES,refusal=error,noSDKOrToolBytesAdded=True,notNativeExecution=True)
 body=json.dumps(result,sort_keys=True,indent=2)+'\n';need(len(body.encode())<=1024**2,'bounded retention manifest');(output/'retention-status.json').write_text(body);return result
def main():
 p=argparse.ArgumentParser();p.add_argument('--source',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();r=collect(a.source,a.output);print(json.dumps(dict(allExistingOutputsRetained=r['allExistingOutputsRetained'],files=len(r['files']),bytes=r['retainedBytes'])));return 0 if r['allExistingOutputsRetained']else 1
if __name__=='__main__':raise SystemExit(main())
