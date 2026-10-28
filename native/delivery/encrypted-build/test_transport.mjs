import test from 'node:test';
import assert from 'node:assert/strict';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import {randomBytes} from 'node:crypto';

const target = new URL('./transport.mjs', import.meta.url);
test('transport implementation exists', () => assert.equal(fs.existsSync(target), true));
if (fs.existsSync(target)) {
 const t = await import(target.href);
 const ctx={repository:'lasmith1689-sys/iphone-native-game-ports-public',ref:'refs/heads/codex/native-cod-loading-renderer-sdk-v13-20261028',purpose:'resource-posix-sdk',bundleId:'00000000-0000-4000-8000-000000000001',sourceClosureSha256:'a'.repeat(64)};
 const key=randomBytes(32);
 const entries=[{name:'source/alpha.c',data:Buffer.from('own synthetic source\n')},{name:'recipe.json',data:Buffer.from('{}\n')}];
 test('deterministic framed/gzip payload and exact closure',()=>{
  const a=t.pack(entries),b=t.pack([...entries].reverse());assert.deepEqual(a,b);
  const parsed=t.unpack(a);assert.deepEqual(parsed.map(x=>x.name),['recipe.json','source/alpha.c']);
  assert.deepEqual(parsed.find(x=>x.name==='source/alpha.c').data,entries[0].data);
 });
 test('GCM authentication/context and role precede extraction',()=>{
  const plain=t.pack(entries),a=t.seal(plain,key,ctx,'input'),b=t.seal(plain,key,ctx,'input');
  assert.notDeepEqual(a,b);assert.deepEqual(t.open(a,key,ctx,'input'),plain);
  assert.throws(()=>t.open(a,randomBytes(32),ctx,'input'));
  for(const at of [0,9,Math.floor(a.length/2),a.length-1]) {const bad=Buffer.from(a);bad[at]^=1;assert.throws(()=>t.open(bad,key,ctx,'input'));}
  for(const changes of [{repository:'other/repo'},{ref:'refs/heads/main'},{purpose:'another-purpose'},{bundleId:'00000000-0000-4000-8000-000000000002'},{sourceClosureSha256:'b'.repeat(64)}])
   assert.throws(()=>t.open(a,key,{...ctx,...changes},'input'));
  assert.throws(()=>t.open(a,key,ctx,'output'));
  assert.throws(()=>t.open(Buffer.concat([a,Buffer.from([0])]),key,ctx,'input'));
 });
 test('unsafe names, duplicate casefold/path prefix and byte corruption rejected',()=>{
  for(const name of ['../escape','/absolute','C:/drive','a\\b','a//b','a/./b','CON.txt','a:stream','a/../b','a/','a.','a '])
   assert.throws(()=>t.pack([{name,data:Buffer.alloc(0)}]),name);
  for(const names of [['a','a'],['A','a'],['a','a/b']])assert.throws(()=>t.pack(names.map(name=>({name,data:Buffer.alloc(0)}))));
  const raw=t.pack(entries);const damaged=Buffer.from(raw);damaged[damaged.length-8]^=8;assert.throws(()=>t.unpack(damaged));
 });
 test('fresh confined extraction and replay ledger, no unauthenticated plaintext writes',()=>{
  const root=fs.mkdtempSync(path.join(os.tmpdir(),'ngp-transport-test-'));
  try {
   const a=t.seal(t.pack(entries),key,ctx,'input'),dest=path.join(root,'plain'),ledger=path.join(root,'consumed.json');
   assert.throws(()=>t.openExtract(a,randomBytes(32),ctx,'input',dest,ledger));assert.equal(fs.existsSync(dest),false);assert.equal(fs.existsSync(ledger),false);
   t.openExtract(a,key,ctx,'input',dest,ledger);assert.equal(fs.readFileSync(path.join(dest,'source/alpha.c'),'utf8'),'own synthetic source\n');
   assert.throws(()=>t.openExtract(a,key,ctx,'input',path.join(root,'replay'),ledger));assert.equal(fs.existsSync(path.join(root,'replay')),false);
   assert.throws(()=>t.extract(t.unpack(t.pack(entries)),dest));
   const outside=fs.mkdtempSync(path.join(os.tmpdir(),'ngp-outside-'));try {fs.symlinkSync(outside,path.join(root,'link'),'junction');assert.throws(()=>t.extract(entries,path.join(root,'link','new')));}finally {fs.rmSync(outside,{recursive:true,force:true});}
  }finally {fs.rmSync(root,{recursive:true,force:true});}
 });
 test('canonical key/metadata and explicit bounds',()=>{
  assert.deepEqual(t.decodeKey(key.toString('base64')),key);
  for(const text of ['',key.toString('hex'),key.toString('base64')+'\n',' '.repeat(44)])assert.throws(()=>t.decodeKey(text));
  assert.throws(()=>t.seal(Buffer.alloc(0),Buffer.alloc(31),ctx,'input'));
  assert.throws(()=>t.seal(Buffer.from('x'),key,{...ctx,unknown:'not-allowed'},'input'));
  assert.throws(()=>t.pack([{name:'x',data:Buffer.alloc(1)}],0));
 });
}
