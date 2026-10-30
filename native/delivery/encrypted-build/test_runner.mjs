import test from 'node:test';
import assert from 'node:assert/strict';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import {randomBytes} from 'node:crypto';
const target=new URL('./run-build.mjs',import.meta.url);
test('private-log build runner exists',()=>assert.equal(fs.existsSync(target),true));
if(fs.existsSync(target)){
 const r=await import(target.href),t=await import('./transport.mjs'),b=await import('./bundle-builder.mjs');
 test('own synthetic successful and failed child logs and outputs sealed, secret not inherited',async()=>{
  const root=fs.mkdtempSync(path.join(os.tmpdir(),'ngp-runner-test-'));const key=randomBytes(32);
  try{
   for(const good of [true,false]){
    const n=good?'pass':'fail',src=path.join(root,n);fs.mkdirSync(src);
    const driver=Buffer.from('import os,sys,json,pathlib\nassert "NGP_INPUT_KEY_20261028" not in os.environ\nprint("PRIVATE_SYNTHETIC_MARKER")\np=pathlib.Path(sys.argv[2]);p.mkdir();(p/"result.json").write_text(json.dumps({"status":"passed"}))\n'+(good?'':'raise RuntimeError("PRIVATE_ERROR_MARKER")\n'));
    fs.writeFileSync(path.join(src,'driver.py'),driver);
    const closure=Buffer.from(t.canonical({files:{'driver.py':{bytes:driver.length,sha256:t.sha(driver)}}}));
    const recipe={format:'NGP-python-sdk-v1',driver:'driver.py',timeoutSeconds:20,outputPaths:['result.json']};
    const ctx={repository:'lasmith1689-sys/iphone-native-game-ports-public',ref:'refs/heads/codex/native-cod-stack-objects-v18-iphone-20261030',purpose:'synthetic-test',bundleId:good?'00000000-0000-4000-8000-000000000001':'00000000-0000-4000-8000-000000000002',sourceClosureSha256:t.sha(closure)};
    const envelope=t.seal(t.pack(b.collect(src,JSON.parse(closure),recipe,closure)),key,ctx,'input');
    const manifest={format:'NGP-public-input-v1',context:ctx,cipherSha256:t.sha(envelope),cipherBytes:envelope.length,sourceFiles:1,sourceBytes:driver.length,recipeSha256:t.sha(Buffer.from(t.canonical(recipe)))};
    const input=path.join(root,n+'.ngp'),publicPath=path.join(root,n+'.json');fs.writeFileSync(input,envelope);fs.writeFileSync(publicPath,t.canonical(manifest));
    const output=path.join(root,n+'-output.ngp');const env={...process.env,GITHUB_REPOSITORY:ctx.repository,GITHUB_REF:ctx.ref,GITHUB_RUN_ID:'1234',GITHUB_RUN_ATTEMPT:'1',GITHUB_SHA:'a'.repeat(40),NGP_INPUT_KEY_20261028:'MUST_NOT_INHERIT'};
    const result=await r.runBuild({input,manifest:publicPath,output,workspace:path.join(root,n+'-work'),python:process.platform==='win32'?'python':'python3'},key,env);
    assert.equal(result.status,good?'passed':'failed');assert(!JSON.stringify(result).includes('PRIVATE_'));
    const outctx={...ctx,runId:'1234',runAttempt:'1',headSha:'a'.repeat(40),inputCipherSha256:manifest.cipherSha256};
    const rows=t.unpack(t.open(fs.readFileSync(output),key,outctx,'output'));
    assert(rows.find(x=>x.name==='logs/stdout.txt').data.toString().includes('PRIVATE_SYNTHETIC_MARKER'));
    assert(rows.find(x=>x.name==='logs/stderr.txt').data.toString().includes('PRIVATE_ERROR_MARKER')===!good);
    assert(!fs.readFileSync(output).includes(Buffer.from('PRIVATE_SYNTHETIC_MARKER')));
    assert.throws(()=>t.open(fs.readFileSync(output),key,{...outctx,runId:'1235'},'output'));
   }
  }finally{key.fill(0);fs.rmSync(root,{recursive:true,force:true});}
 });
}
