import test from 'node:test';
import assert from 'node:assert/strict';
import * as fs from 'node:fs';
import * as path from 'node:path';
import * as os from 'node:os';
import {randomBytes} from 'node:crypto';
const target=new URL('./decrypt.mjs',import.meta.url);
test('independently hash-bound decrypt helper exists',()=>assert.equal(fs.existsSync(target),true));
if(fs.existsSync(target)){
 const d=await import(target.href),t=await import('./transport.mjs');
 test('expected outer hash/size and exact output context required before filesystem extraction',()=>{
  const root=fs.mkdtempSync(path.join(os.tmpdir(),'ngp-decrypt-test-')),key=randomBytes(32);
  try{
   const ctx={repository:'lasmith1689-sys/iphone-native-game-ports-public',ref:'refs/heads/codex/native-cod-loading-renderer-sdk-v13-20261028',purpose:'test',bundleId:'00000000-0000-4000-8000-000000000001',sourceClosureSha256:'a'.repeat(64)};
   const input=t.seal(t.pack([{name:'own.txt',data:Buffer.from('synthetic')}]),key,ctx,'input');fs.writeFileSync(path.join(root,'input'),input);fs.writeFileSync(path.join(root,'ctx'),t.canonical(ctx));
   const opt={input:path.join(root,'input'),context:path.join(root,'ctx'),role:'input','expected-sha256':t.sha(input),'expected-size':String(input.length),output:path.join(root,'plain'),ledger:path.join(root,'ledger')};
   for(const change of [{'expected-sha256':'0'.repeat(64)},{'expected-size':String(input.length+1)},{role:'output'}]){assert.throws(()=>d.decrypt({...opt,...change},key));assert(!fs.existsSync(opt.output));}
   assert.equal(d.decrypt(opt,key).files,1);assert.equal(fs.readFileSync(path.join(opt.output,'own.txt'),'utf8'),'synthetic');
  }finally{key.fill(0);fs.rmSync(root,{recursive:true,force:true});}
 });
}
