/* No target execution/signing. Independent expected ciphertext digest and size
 * must come from the reviewed input or recorded CI/artifact evidence, not from
 * an untrusted adjacent metadata file. Output context includes actual run/head.
 */
import * as fs from 'node:fs';
import * as path from 'node:path';
import {fileURLToPath} from 'node:url';
import {sha,openExtract,MAX_BYTES,fail} from './transport.mjs';
import {args,readKey} from './bundle-builder.mjs';
export function decrypt(a,key){
 if(!/^[a-f0-9]{64}$/.test(a['expected-sha256'])||!/^[1-9][0-9]{0,9}$/.test(a['expected-size']))fail();
 const size=Number(a['expected-size']);if(size>MAX_BYTES+16412||!fs.statSync(a.input).isFile()||fs.statSync(a.input).size!==size)fail();
 const envelope=fs.readFileSync(a.input);if(sha(envelope)!==a['expected-sha256'])fail();
 const context=JSON.parse(fs.readFileSync(a.context,'utf8'));
 const root=openExtract(envelope,key,context,a.role,a.output,a.ledger);let files=0,bytes=0;
 function walk(dir){for(const p of fs.readdirSync(dir,{withFileTypes:true})){const f=path.join(dir,p.name);if(p.isDirectory())walk(f);else{if(!p.isFile()||p.isSymbolicLink())fail();files++;bytes+=fs.statSync(f).size;}}}
 walk(root);return {status:'authenticated-and-extracted',cipherSha256:sha(envelope),files,bytes};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 let key;try{
  if(!process.versions.node.startsWith('24.'))fail();
  const a=args(process.argv.slice(2),['input','context','role','expected-sha256','expected-size','output','ledger']);key=readKey();console.log(JSON.stringify(decrypt(a,key)));
 }catch{console.error('Authenticated decryption rejected; no plaintext diagnostic is printed.');process.exitCode=1;}finally{key?.fill(0);}
}
