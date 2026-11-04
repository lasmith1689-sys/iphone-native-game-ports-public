/* Deterministic exact source closure. Excludes every file not explicitly listed.
 * Production keys are supplied by caller via stdin or the named environment;
 * this module never generates/installs a production key or accesses a provider.
 */
import * as fs from 'node:fs';
import * as path from 'node:path';
import {fileURLToPath} from 'node:url';
import {canonical,sha,pack,seal,safeName,noSymlinkParents,decodeKey,validateContext,fail,MAX_BYTES,MAX_FILES} from './transport.mjs';
function need(v){if(!v)fail();}
export function validateRecipe(recipe,closure){
 need(recipe&&Object.keys(recipe).sort().join('|')==='driver|format|outputPaths|timeoutSeconds');
 need(recipe.format==='NGP-python-sdk-v1');safeName(recipe.driver);need(recipe.driver.endsWith('.py')&&Object.hasOwn(closure.files,recipe.driver));
 need(Number.isSafeInteger(recipe.timeoutSeconds)&&recipe.timeoutSeconds>=1&&recipe.timeoutSeconds<=7200);
 need(Array.isArray(recipe.outputPaths)&&recipe.outputPaths.length>0&&recipe.outputPaths.length<=64);
 for(const p of recipe.outputPaths)safeName(p);
 need(new Set(recipe.outputPaths.map(x=>x.toLowerCase())).size===recipe.outputPaths.length);
 return recipe;
}
export function collect(root,closure,recipe,closureRaw=Buffer.from(canonical(closure))){
 root=noSymlinkParents(root);need(fs.statSync(root).isDirectory());
 need(closure&&closure.files&&typeof closure.files==='object'&&!Array.isArray(closure.files));
 validateRecipe(recipe,closure);const fileNames=Object.keys(closure.files).sort();need(fileNames.length>0&&fileNames.length<=MAX_FILES-2);
 let total=0;const rows=[];
 for(const name of fileNames){
  safeName(name);const expected=closure.files[name];need(expected&&Number.isSafeInteger(expected.bytes)&&expected.bytes>=0&&/^[a-f0-9]{64}$/.test(expected.sha256));
  total+=expected.bytes;need(total<=MAX_BYTES-8*1024*1024);
  const p=noSymlinkParents(path.join(root,...name.split('/')));need(p.startsWith(root+path.sep));
  const st=fs.lstatSync(p);need(st.isFile()&&!st.isSymbolicLink()&&st.size===expected.bytes);
  const data=fs.readFileSync(p);need(data.length===expected.bytes&&sha(data)===expected.sha256);rows.push({name:'source/'+name,data});
 }
 need(Buffer.isBuffer(closureRaw)&&canonical(JSON.parse(closureRaw.toString('utf8')))===canonical(closure));
 rows.push({name:'source-closure.json',data:closureRaw},{name:'recipe.json',data:Buffer.from(canonical(recipe))});
 // pack performs global duplicate/casefold/ancestor checks for all virtual names.
 pack(rows);return rows;
}
export function readKey(){
 let text=process.env.NGP_INPUT_KEY_20261028;delete process.env.NGP_INPUT_KEY_20261028;
 if(text===undefined){const raw=fs.readFileSync(0);need(raw.length<=128);text=raw.toString('utf8');if(text.endsWith('\n'))text=text.slice(0,-1);}
 return decodeKey(text);
}
export function args(argv,required){
 const result={};for(let k=0;k<argv.length;k+=2){need(argv[k]?.startsWith('--')&&k+1<argv.length);const n=argv[k].slice(2);need(required.includes(n)&&!Object.hasOwn(result,n));result[n]=argv[k+1];}
 need(required.every(k=>Object.hasOwn(result,k)));return result;
}
export function buildBundle(root,closurePath,recipePath,contextPath,destination,key){
 const closureRaw=fs.readFileSync(closurePath),closure=JSON.parse(closureRaw.toString('utf8'));
 const recipe=JSON.parse(fs.readFileSync(recipePath,'utf8')),context=JSON.parse(fs.readFileSync(contextPath,'utf8'));
 validateContext(context,'input');need(context.sourceClosureSha256===sha(closureRaw));
 const rows=collect(root,closure,recipe,closureRaw),plain=pack(rows);let envelope;
 try{envelope=seal(plain,key,context,'input');}finally{plain.fill(0);}
 const publicManifest={format:'NGP-public-input-v1',context,cipherSha256:sha(envelope),cipherBytes:envelope.length,
  sourceFiles:Object.keys(closure.files).length,sourceBytes:Object.values(closure.files).reduce((a,x)=>a+x.bytes,0),recipeSha256:sha(Buffer.from(canonical(recipe)))};
 const dest=noSymlinkParents(destination);need(!fs.existsSync(dest)&&fs.statSync(path.dirname(dest)).isDirectory());fs.mkdirSync(dest,{mode:0o700});
 fs.writeFileSync(path.join(dest,'input.ngp'),envelope,{flag:'wx',mode:0o600});
 fs.writeFileSync(path.join(dest,'public-manifest.json'),canonical(publicManifest)+'\n',{flag:'wx',mode:0o600});
 return publicManifest;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 let key;try{
  need(process.versions.node.startsWith('24.'));const a=args(process.argv.slice(2),['root','closure','recipe','context','output']);key=readKey();
  const m=buildBundle(a.root,a.closure,a.recipe,a.context,a.output,key);
  console.log(JSON.stringify({status:'sealed',cipherSha256:m.cipherSha256,cipherBytes:m.cipherBytes,sourceFiles:m.sourceFiles,sourceBytes:m.sourceBytes}));
 }catch{console.error('Encrypted source bundle preparation failed; no source diagnostic is printed.');process.exitCode=1;}finally{key?.fill(0);}
}
