/* Public runner wrapper. Only generic status/digests may reach live CI output.
 * Decrypted trusted source runs without inheriting the transport secret or
 * Actions credential environment. Its stdout/stderr go directly to private
 * mode0600 files and are included only inside an authenticated output envelope.
 * Each child stream is capped at32MiB; exceeding it kills the process group and
 * seals a failed result and bounded logs, rather than filling the runner disk.
 */
import * as fs from 'node:fs';
import * as path from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawn} from 'node:child_process';
import {canonical,sha,seal,openExtract,pack,safeName,noSymlinkParents,validateContext,fail,MAX_BYTES} from './transport.mjs';
import {args,readKey,validateRecipe} from './bundle-builder.mjs';
export const REPOSITORY='lasmith1689-sys/iphone-native-game-ports-public';
export const REF='refs/heads/codex/native-cod-stack-objects-v18-mac-20261029';
function need(v){if(!v)fail();}
export function validateManifest(m){
 need(m&&Object.keys(m).sort().join('|')==='cipherBytes|cipherSha256|context|format|recipeSha256|sourceBytes|sourceFiles');
 need(m.format==='NGP-public-input-v1');validateContext(m.context,'input');
 need(m.context.repository===REPOSITORY&&m.context.ref===REF);
 need(/^[a-f0-9]{64}$/.test(m.cipherSha256)&&/^[a-f0-9]{64}$/.test(m.recipeSha256));
 need(Number.isSafeInteger(m.cipherBytes)&&m.cipherBytes>=28&&m.cipherBytes<=MAX_BYTES+16412);
 need(Number.isSafeInteger(m.sourceFiles)&&m.sourceFiles>0&&m.sourceFiles<=19998&&Number.isSafeInteger(m.sourceBytes)&&m.sourceBytes>=0&&m.sourceBytes<MAX_BYTES);
 return m;
}
function verifySources(plain,closure,m){
 const expected=Object.keys(closure.files).sort();need(expected.length===m.sourceFiles);let total=0;
 for(const name of expected){safeName(name);const row=closure.files[name];need(row&&Number.isSafeInteger(row.bytes)&&row.bytes>=0&&/^[a-f0-9]{64}$/.test(row.sha256));
  const p=noSymlinkParents(path.join(plain,'source',...name.split('/')));need(fs.lstatSync(p).isFile());const raw=fs.readFileSync(p);need(raw.length===row.bytes&&sha(raw)===row.sha256);total+=raw.length;
 }need(total===m.sourceBytes);
 const actual=[];function walk(dir,prefix){for(const e of fs.readdirSync(dir,{withFileTypes:true})){const n=prefix+e.name;safeName(n);const p=path.join(dir,e.name);need(!e.isSymbolicLink());if(e.isDirectory())walk(p,n+'/');else{need(e.isFile());actual.push(n);}}}
 walk(plain,'');const wanted=['recipe.json','source-closure.json',...expected.map(x=>'source/'+x)].sort();need(canonical(actual.sort())===canonical(wanted));
}
export function childEnvironment(env){
 const child={};for(const key of ['PATH','HOME','TMPDIR','TMP','TEMP','SYSTEMROOT','WINDIR','PATHEXT','COMSPEC','USERPROFILE','LOCALAPPDATA'])if(env[key]!==undefined)child[key]=env[key];
 child.PYTHONNOUSERSITE='1';child.PYTHONDONTWRITEBYTECODE='1';child.LC_ALL='C';return child;
}
function child(python,argv,cwd,env,stdout,stderr,timeout){
 return new Promise(resolve=>{
  const out=fs.openSync(stdout,'wx',0o600),err=fs.openSync(stderr,'wx',0o600);let timer,finished=false,logLimit=false;
  const finish=result=>{if(finished)return;finished=true;clearTimeout(timer);fs.closeSync(out);fs.closeSync(err);resolve(result);};
  let p;const kill=()=>{try{if(process.platform!=='win32')process.kill(-p.pid,'SIGKILL');else p.kill('SIGKILL');}catch{}};
  try{p=spawn(python,argv,{cwd,env,stdio:['ignore','pipe','pipe'],shell:false,detached:process.platform!=='win32',windowsHide:true});}
  catch{finish({code:-1,signal:'spawn-error'});return;}
  for(const [stream,fd] of [[p.stdout,out],[p.stderr,err]]){let bytes=0;stream.on('data',chunk=>{
   if(finished)return;const remaining=32*1024*1024-bytes;if(remaining>0){const part=chunk.subarray(0,remaining);fs.writeSync(fd,part);bytes+=part.length;}
   if(chunk.length>remaining){logLimit=true;kill();}
  });}
  p.once('error',()=>finish({code:-1,signal:'spawn-error'}));p.once('close',(code,signal)=>finish({code:logLimit?-2:(code??-1),signal:logLimit?'log-limit':(signal??null)}));
  timer=setTimeout(kill,timeout*1000);
 });
}
function outputRows(root,selected,requireAll){
 let total=0;const rows=[],seen=new Set();
 function add(p,name){safeName(name);need(!seen.has(name.toLowerCase()));seen.add(name.toLowerCase());noSymlinkParents(p);const st=fs.lstatSync(p);
  if(st.isDirectory()){for(const entry of fs.readdirSync(p).sort())add(path.join(p,entry),name+'/'+entry);return;}
  need(st.isFile()&&!st.isSymbolicLink());total+=st.size;need(total<MAX_BYTES-8*1024*1024);rows.push({name:'outputs/'+name,data:fs.readFileSync(p)});
 }
 for(const relative of selected){safeName(relative);const p=path.join(root,...relative.split('/'));if(fs.existsSync(p))add(p,relative);else need(!requireAll);}
 return rows;
}
export async function runBuild(options,key,env=process.env){
 need(process.versions.node.startsWith('24.'));const m=validateManifest(JSON.parse(fs.readFileSync(options.manifest,'utf8')));
 need(env.GITHUB_REPOSITORY===REPOSITORY&&env.GITHUB_REF===REF);
 const outctx={...m.context,runId:env.GITHUB_RUN_ID,runAttempt:env.GITHUB_RUN_ATTEMPT,headSha:env.GITHUB_SHA,inputCipherSha256:m.cipherSha256};validateContext(outctx,'output');
 need(fs.statSync(options.input).isFile()&&fs.statSync(options.input).size===m.cipherBytes);
 const input=fs.readFileSync(options.input);need(input.length===m.cipherBytes&&sha(input)===m.cipherSha256);
 const work=noSymlinkParents(options.workspace),output=noSymlinkParents(options.output);need(!fs.existsSync(work)&&!fs.existsSync(output));
 need(!output.startsWith(work+path.sep));fs.mkdirSync(work,{mode:0o700});
 const plain=path.join(work,'plaintext');openExtract(input,key,m.context,'input',plain,path.join(work,'input-consumed.json'));
 const stdout=path.join(work,'stdout.txt'),stderr=path.join(work,'stderr.txt');const result={format:'NGP-private-build-result-v1',status:'failed',inputCipherSha256:m.cipherSha256,context:outctx};
 let closure,recipe,rows=[];
 try{
  const raw=fs.readFileSync(path.join(plain,'source-closure.json'));need(sha(raw)===m.context.sourceClosureSha256);closure=JSON.parse(raw);
  recipe=JSON.parse(fs.readFileSync(path.join(plain,'recipe.json'),'utf8'));need(sha(Buffer.from(canonical(recipe)))===m.recipeSha256);validateRecipe(recipe,closure);verifySources(plain,closure,m);
  const probe=path.join(work,'probe-output');const execution=await child(options.python??'python3',[path.join(plain,'source',...recipe.driver.split('/')),'--output',probe],work,childEnvironment(env),stdout,stderr,recipe.timeoutSeconds);
  result.child=execution;verifySources(plain,closure,m);need(execution.code===0);
  const proof=JSON.parse(fs.readFileSync(path.join(probe,'result.json'),'utf8'));need(proof.status==='passed');
  rows=outputRows(probe,recipe.outputPaths,true);result.status='passed';
 }catch(error){
  // This diagnostic stays encrypted. Never echo a child traceback/source line.
  fs.writeFileSync(path.join(work,'wrapper-error.txt'),String(error?.stack??error),{flag:'wx',mode:0o600});
  try{if(recipe)rows=outputRows(path.join(work,'probe-output'),recipe.outputPaths,false);}catch{rows=[];}
 }
 for(const [name,p] of [['logs/stdout.txt',stdout],['logs/stderr.txt',stderr],['logs/wrapper-error.txt',path.join(work,'wrapper-error.txt')]])if(fs.existsSync(p)){
  const st=fs.statSync(p);need(st.isFile()&&st.size<=32*1024*1024);rows.push({name,data:fs.readFileSync(p)});
 }
 rows.push({name:'build-result.json',data:Buffer.from(canonical(result))});
 const payload=pack(rows);let sealed;try{sealed=seal(payload,key,outctx,'output');}finally{payload.fill(0);}
 fs.writeFileSync(output,sealed,{flag:'wx',mode:0o600});
 return {status:result.status,sealed:true,cipherSha256:sha(sealed),cipherBytes:sealed.length,inputCipherSha256:m.cipherSha256};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 let key;try{
  const a=args(process.argv.slice(2),['input','manifest','output','workspace']);key=readKey();
  const result=await runBuild(a,key);console.log(JSON.stringify(result));
  if(process.env.GITHUB_OUTPUT){noSymlinkParents(process.env.GITHUB_OUTPUT);fs.appendFileSync(process.env.GITHUB_OUTPUT,`sealed=true\nstatus=${result.status}\ncipherSha256=${result.cipherSha256}\n`);}
 }catch{console.error('Encrypted build wrapper failed; no plaintext diagnostic is printed.');process.exitCode=1;}finally{key?.fill(0);}
}
