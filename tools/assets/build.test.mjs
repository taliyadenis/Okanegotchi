import { test } from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { PNG } from 'pngjs'
import { root, decodeSource, rgb, rgb565 } from './build.mjs'
const manifest=JSON.parse(fs.readFileSync(path.join(root,'assets/manifest.json'),'utf8'))
const header=fs.readFileSync(path.join(root,'firmware/generated/okanegotchi_assets.h'),'utf8')
function cArray(name) {
  const start=header.indexOf(' '+name+'[')
  assert.notEqual(start,-1,name+' exists')
  return header.slice(header.indexOf('{',start)+1,header.indexOf('};',start)).split(',').map(v=>v.trim()).filter(Boolean).map(Number)
}
for(const [pet,preset] of Object.entries(manifest.presets)) {
  test(pet+' firmware palette follows RGB565 bit packing',()=>assert.deepEqual(cArray(pet+'_palette'),preset.palette.map(rgb565)))
  for(const [state,a] of Object.entries(manifest.animations)) {
    test(pet+' '+state+': source, web, indexed firmware, mask and RGB565 agree',()=>{
      const name=pet+'_'+state
      const {png:source}=decodeSource(fs.readFileSync(path.join(root,'assets/source',pet,a.file)))
      const web=PNG.sync.read(fs.readFileSync(path.join(root,'public/assets/v1',name+'.png')))
      assert.equal(web.width,32*a.frames);assert.equal(web.height,32)
      const indices=fs.readFileSync(path.join(root,'public/assets/v1',name+'.bin'))
      assert.deepEqual(Array.from(indices),cArray(name+'_indices'))
      const mask=cArray(name+'_mask'), direct=cArray(name+'_rgb565')
      for(let f=0;f<a.frames;f++) for(let y=0;y<32;y++) for(let x=0;x<32;x++) {
        const i=f*1024+y*32+x,p=(y*web.width+f*32+x)*4
        assert.deepEqual(web.data.subarray(p,p+4),source.data.subarray(p,p+4))
        const opaque=source.data[p+3]===255
        assert.equal(Boolean(mask[i>>3]&(1<<(7-(i&7)))),opaque)
        assert.equal(indices[i]!==0,opaque)
        if(opaque) assert.deepEqual(rgb(preset.palette[indices[i]]),Array.from(source.data.subarray(p,p+3)))
        const [r,g,b]=opaque?Array.from(source.data.subarray(p,p+3)):[0,0,0]
        assert.equal(direct[i],((r>>3)<<11)|((g>>2)<<5)|(b>>3))
      }
    })
  }
}
test('decoder rejects invalid or truncated input',()=>{
  assert.throws(()=>decodeSource(Buffer.from('not a PNG')))
  const source=fs.readFileSync(path.join(root,'assets/source/gator/idle.png'))
  assert.throws(()=>decodeSource(source.subarray(0,source.length-20)))
})
