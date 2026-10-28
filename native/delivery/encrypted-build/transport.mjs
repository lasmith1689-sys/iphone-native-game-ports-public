/* Node24 built-ins only. No plaintext filesystem write before GCM final().
 * Deliberately bounded/in-memory: at most512MiB decrypted compressed payload,
 * at most512MiB expanded files. This is not a multi-gigabyte streaming format.
 * Caller owns trusted, serialized destinations and key lifetime; same-user
 * hostile process/privileged runner compromise is outside this interface.
 */
import * as crypto from 'node:crypto';
import * as fs from 'node:fs';
import * as path from 'node:path';
import * as zlib from 'node:zlib';
export const MAX_BYTES=512*1024*1024,MAX_FILES=20000,MAX_HEADER=16384;
const MAGIC=Buffer.from('NGPENC01'),BUNDLE=Buffer.from('NGPBDL01');
export function fail(){throw new Error('private build transport rejected');}
function need(v){if(!v)fail();}
export function sha(data){return crypto.createHash('sha256').update(data).digest('hex');}
export function canonical(v){
 if(v===null||typeof v==='string'||typeof v==='boolean')return JSON.stringify(v);
 if(typeof v==='number'){need(Number.isSafeInteger(v));return String(v);}
 if(Array.isArray(v))return '['+v.map(canonical).join(',')+']';
 need(v&&typeof v==='object'&&Object.getPrototypeOf(v)===Object.prototype);
 return '{'+Object.keys(v).sort().map(k=>JSON.stringify(k)+':'+canonical(v[k])).join(',')+'}';
}
function exact(obj,keys){need(obj&&typeof obj==='object'&&!Array.isArray(obj));need(Object.keys(obj).sort().join('|')===[...keys].sort().join('|'));}
export function decodeKey(text){
 need(typeof text==='string'&&/^[A-Za-z0-9+/]{43}=$/.test(text));
 const key=Buffer.from(text,'base64');need(key.length===32&&key.toString('base64')===text);return key;
}
export function validateContext(ctx,role){
 need(role==='input'||role==='output');
 const common=['repository','ref','purpose','bundleId','sourceClosureSha256'];
 exact(ctx,role==='input'?common:[...common,'runId','runAttempt','headSha','inputCipherSha256']);
 need(/^[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+$/.test(ctx.repository));
 need(/^refs\/heads\/[A-Za-z0-9_./-]{1,160}$/.test(ctx.ref)&&!ctx.ref.includes('..'));
 need(/^[a-z0-9-]{1,80}$/.test(ctx.purpose));
 need(/^[a-f0-9]{8}-[a-f0-9]{4}-4[a-f0-9]{3}-[89ab][a-f0-9]{3}-[a-f0-9]{12}$/.test(ctx.bundleId));
 need(/^[a-f0-9]{64}$/.test(ctx.sourceClosureSha256));
 if(role==='output')need(/^[1-9][0-9]{0,19}$/.test(ctx.runId)&&/^[1-9][0-9]{0,5}$/.test(ctx.runAttempt)&&/^[a-f0-9]{40}$/.test(ctx.headSha)&&/^[a-f0-9]{64}$/.test(ctx.inputCipherSha256));
 return ctx;
}
function derived(key,role){need(Buffer.isBuffer(key)&&key.length===32);return Buffer.from(crypto.hkdfSync('sha256',key,Buffer.alloc(0),Buffer.from('NGPENC01:'+role),32));}
export function seal(plain,key,context,role){
 validateContext(context,role);need(Buffer.isBuffer(plain)&&plain.length<=MAX_BYTES);
 const header={format:'NGP-AES256GCM-v1',role,context,nonce:crypto.randomBytes(12).toString('hex'),plaintextBytes:plain.length,plaintextSha256:sha(plain)};
 const h=Buffer.from(canonical(header));need(h.length<=MAX_HEADER);
 const length=Buffer.alloc(4);length.writeUInt32BE(h.length);const aad=Buffer.concat([MAGIC,length,h]);
 const subkey=derived(key,role);try {
  const cipher=crypto.createCipheriv('aes-256-gcm',subkey,Buffer.from(header.nonce,'hex'),{authTagLength:16});cipher.setAAD(aad);
  return Buffer.concat([aad,cipher.update(plain),cipher.final(),cipher.getAuthTag()]);
 }finally{subkey.fill(0);}
}
export function open(envelope,key,context,role){
 validateContext(context,role);need(Buffer.isBuffer(envelope)&&envelope.length>=28&&envelope.length<=MAX_BYTES+MAX_HEADER+28);
 need(envelope.subarray(0,8).equals(MAGIC));const n=envelope.readUInt32BE(8);need(n>0&&n<=MAX_HEADER&&12+n+16<=envelope.length);
 let header;try{header=JSON.parse(envelope.subarray(12,12+n).toString('utf8'));}catch{fail();}
 exact(header,['format','role','context','nonce','plaintextBytes','plaintextSha256']);
 need(Buffer.from(canonical(header)).equals(envelope.subarray(12,12+n)));
 need(header.format==='NGP-AES256GCM-v1'&&header.role===role&&canonical(header.context)===canonical(context));
 need(/^[a-f0-9]{24}$/.test(header.nonce)&&/^[a-f0-9]{64}$/.test(header.plaintextSha256));
 need(Number.isSafeInteger(header.plaintextBytes)&&header.plaintextBytes>=0&&header.plaintextBytes<=MAX_BYTES&&envelope.length===12+n+header.plaintextBytes+16);
 const subkey=derived(key,role);let provisional;
 try{
  const decipher=crypto.createDecipheriv('aes-256-gcm',subkey,Buffer.from(header.nonce,'hex'),{authTagLength:16});
  decipher.setAAD(envelope.subarray(0,12+n));decipher.setAuthTag(envelope.subarray(-16));
  provisional=decipher.update(envelope.subarray(12+n,-16));const tail=decipher.final();
  need(tail.length===0&&provisional.length===header.plaintextBytes&&sha(provisional)===header.plaintextSha256);
  return provisional;
 }catch{if(provisional)provisional.fill(0);fail();}finally{subkey.fill(0);}
}
export function safeName(name){
 need(typeof name==='string'&&name.length>0&&name.length<=512&&/^[A-Za-z0-9_./-]+$/.test(name));
 const parts=name.split('/');for(const p of parts)need(p&&p!=='.'&&p!=='..'&&p.length<=128&&!/[. ]$/.test(p)&&!/^(con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\.|$)/i.test(p));
 return name;
}
function names(rows){
 need(Array.isArray(rows)&&rows.length>0&&rows.length<=MAX_FILES);const seen=new Set();
 for(const row of rows){safeName(row.name);const name=row.name.toLowerCase();need(!seen.has(name));seen.add(name);}
 for(const name of seen){let at=name.indexOf('/');while(at>=0){need(!seen.has(name.slice(0,at)));at=name.indexOf('/',at+1);}}
}
export function pack(entries,max=MAX_BYTES){
 need(Number.isSafeInteger(max)&&max>=0&&max<=MAX_BYTES);names(entries);
 const sorted=[...entries].sort((a,b)=>a.name<b.name?-1:a.name>b.name?1:0);let total=0;
 const files=sorted.map(x=>{need(Buffer.isBuffer(x.data));total+=x.data.length;need(total<=max);return {name:x.name,bytes:x.data.length,sha256:sha(x.data)};});
 const manifest=Buffer.from(canonical({format:'NGP-bundle-v1',files}));need(manifest.length<=8*1024*1024);
 const length=Buffer.alloc(4);length.writeUInt32BE(manifest.length);need(total+12+manifest.length<=max);
 const raw=Buffer.concat([BUNDLE,length,manifest,...sorted.map(x=>x.data)]);
 const compressed=zlib.gzipSync(raw,{level:9,mtime:0});need(compressed.length<=MAX_BYTES);return compressed;
}
export function unpack(compressed){
 need(Buffer.isBuffer(compressed)&&compressed.length<=MAX_BYTES);
 let raw;try{raw=zlib.gunzipSync(compressed,{maxOutputLength:MAX_BYTES});}catch{fail();}
 need(raw.length>=12&&raw.subarray(0,8).equals(BUNDLE));const n=raw.readUInt32BE(8);need(n>0&&n<=8*1024*1024&&12+n<=raw.length);
 let m;try{m=JSON.parse(raw.subarray(12,12+n).toString('utf8'));}catch{fail();}
 exact(m,['format','files']);need(m.format==='NGP-bundle-v1'&&Buffer.from(canonical(m)).equals(raw.subarray(12,12+n)));names(m.files);
 let at=12+n,last='';const rows=[];for(const f of m.files){
  exact(f,['name','bytes','sha256']);need(last<f.name);last=f.name;
  need(Number.isSafeInteger(f.bytes)&&f.bytes>=0&&f.bytes<=raw.length-at&&/^[a-f0-9]{64}$/.test(f.sha256));
  const data=raw.subarray(at,at+f.bytes);need(sha(data)===f.sha256);at+=f.bytes;rows.push({name:f.name,data});
 }need(at===raw.length);return rows;
}
export function noSymlinkParents(target){
 const absolute=path.resolve(target);let current=absolute;
 for(;;){if(fs.existsSync(current))need(!fs.lstatSync(current).isSymbolicLink());const parent=path.dirname(current);if(parent===current)break;current=parent;}
 return absolute;
}
export function extract(entries,destination){
 names(entries);const root=noSymlinkParents(destination);need(!fs.existsSync(root)&&fs.statSync(path.dirname(root)).isDirectory());
 fs.mkdirSync(root,{mode:0o700});for(const f of entries){
  need(Buffer.isBuffer(f.data));const output=path.join(root,...f.name.split('/'));need(output.startsWith(root+path.sep));
  fs.mkdirSync(path.dirname(output),{recursive:true,mode:0o700});noSymlinkParents(output);fs.writeFileSync(output,f.data,{flag:'wx',mode:0o600});
 }return root;
}
export function openExtract(envelope,key,context,role,destination,ledger){
 // Authentication and every expanded filename/hash are verified before ledger
 // or destination is created. Ledger is one-shot even when extraction fails.
 const plaintext=open(envelope,key,context,role);let rows;
 try{rows=unpack(plaintext);}finally{plaintext.fill(0);}
 // unpack owns a separate expanded buffer; clearing compressed bytes is safe.
 const target=noSymlinkParents(destination),record=noSymlinkParents(ledger);
 need(!fs.existsSync(target)&&!fs.existsSync(record)&&record!==target&&!record.startsWith(target+path.sep));
 fs.writeFileSync(record,canonical({format:'NGP-consumed-v1',context,role,cipherSha256:sha(envelope)}),{flag:'wx',mode:0o600});
 return extract(rows,target);
}
