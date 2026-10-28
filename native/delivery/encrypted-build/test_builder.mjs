import test from 'node:test';
import assert from 'node:assert/strict';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
const target=new URL('./bundle-builder.mjs',import.meta.url);
test('exact closure builder exists',()=>assert.equal(fs.existsSync(target),true));
if(fs.existsSync(target)){
 const b=await import(target.href),t=await import('./transport.mjs');
 test('only exact explicitly listed hash-bound regular files enter bundle',()=>{
  const root=fs.mkdtempSync(path.join(os.tmpdir(),'ngp-builder-test-'));
  try{
   fs.mkdirSync(path.join(root,'native'));fs.writeFileSync(path.join(root,'native','own.py'),'print("synthetic")\n');fs.writeFileSync(path.join(root,'extra.txt'),'MUST NOT ENTER');
   const raw=fs.readFileSync(path.join(root,'native','own.py'));
   const closure={files:{'native/own.py':{bytes:raw.length,sha256:t.sha(raw)}}};
   const recipe={format:'NGP-python-sdk-v1',driver:'native/own.py',timeoutSeconds:30,outputPaths:['result.json']};
   const rows=b.collect(root,closure,recipe);assert.deepEqual(rows.map(x=>x.name).sort(),['recipe.json','source-closure.json','source/native/own.py']);
   assert.deepEqual(b.collect(root,closure,recipe),rows);
   assert.throws(()=>b.collect(root,{files:{'../escape':{bytes:0,sha256:'0'.repeat(64)}}},recipe));
   assert.throws(()=>b.collect(root,{files:{'extra.txt':{bytes:1,sha256:'0'.repeat(64)}}},recipe));
   assert.throws(()=>b.collect(root,closure,{...recipe,driver:'unlisted.py'}));
   fs.mkdirSync(path.join(root,'real'));fs.writeFileSync(path.join(root,'real','own.py'),raw);fs.symlinkSync(path.join(root,'real'),path.join(root,'linked'),'junction');assert.throws(()=>b.collect(root,{files:{'linked/own.py':{bytes:raw.length,sha256:t.sha(raw)}}},{...recipe,driver:'linked/own.py'}));
  }finally{fs.rmSync(root,{recursive:true,force:true});}
 });
}
