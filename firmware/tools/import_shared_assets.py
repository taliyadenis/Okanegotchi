"""Validate every uploaded pixel against the team's generated header; emit runtime registry.

Run npm run assets:build first after editing shared sources. Include the header in
one translation unit only. Images are split logically at x=frame*32, not on-device.
"""
import json
from pathlib import Path
import re
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
STATES=('idle','blink','pat','needs_checkin','sleepy','ghost','eating','traveling','celebrate','neutral','revive')
def build(export_frames=False):
    shared=ROOT.parent/'assets';m=json.loads((shared/'manifest.json').read_text(encoding="utf-8"));header=(ROOT/'generated/okanegotchi_assets.h').read_text(encoding="utf-8")
    if m['canvas']!={'width':32,'height':32} or m['asset_version']!=1:raise ValueError('unsupported canvas/version')
    def array(name):
        found=re.search(r'\b'+re.escape(name)+r'\[\d+\]\s*=\s*\{([^}]+)\}',header)
        if not found:raise ValueError('missing generated array '+name)
        return [int(s) for s in found[1].split(',') if s.strip()]
    lines=['// Generated from the shared website asset manifest.','#include "../assets.hpp"','#include "../../generated/okanegotchi_assets.h"','namespace okanegachi::assets {','namespace art=::okanegotchi;']
    rows=[];frames=[];colors=0;total=0
    for pet,preset in m['presets'].items():
        if not re.fullmatch('[a-z][a-z0-9_]*',pet):raise ValueError('unsafe preset ID')
        palette=array(pet+'_palette');colors+=len(palette)*2
        expected=[0 if c is None else ((int(c[1:3],16)>>3)<<11)|((int(c[3:5],16)>>2)<<5)|(int(c[5:7],16)>>3) for c in preset['palette']]
        if palette!=expected:raise ValueError('stale generated palette; run npm run assets:build')
        for state,clip in m['animations'].items():
            if not re.fullmatch('[a-z][a-z0-9_]*',state):raise ValueError('unsafe animation ID')
            if not 1<=clip['frames']<=32 or not 25<=clip['frame_ms']<=5000:raise ValueError('timing/frame bounds')
            name=pet+'_'+state;im=Image.open(shared/'source'/pet/clip['file']).convert('RGBA')
            if im.size!=(32*clip['frames'],32):raise ValueError('sheet dimensions')
            indices=array(name+'_indices');mask=array(name+'_mask')
            if len(indices)!=clip['frames']*1024 or len(mask)!=clip['frames']*128:raise ValueError('generated length')
            if not re.search(r'\b'+name+r'_frame_ms = '+str(clip['frame_ms'])+';',header):raise ValueError('stale generated timing')
            for f in range(clip['frames']):
                crop=im.crop((f*32,0,f*32+32,32));frames.append(crop)
                if export_frames:
                    directory=ROOT/'build/assets/frames'/pet/state;directory.mkdir(parents=True,exist_ok=True);crop.save(directory/f'{f:03}.png')
                for n,pixel in enumerate(crop.getdata()):
                    offset=f*1024+n;index=indices[offset]
                    if index>=len(preset['palette']):raise ValueError('palette index outside table')
                    color=preset['palette'][index];want=(0,0,0,0) if color is None else tuple(bytes.fromhex(color[1:]))+(255,)
                    if pixel[3] not in (0,255) or (pixel[3] and pixel!=want) or (not pixel[3] and index!=0):raise ValueError('generated pixel differs from source')
                    if bool(mask[offset//8]&(0x80>>(offset%8)))!=bool(pixel[3]):raise ValueError('alpha mask differs')
                lines.append(f'static const Frame {name}_{f}={{"{name}.{f}",32,32,2,art::{name}_indices+{f*1024},nullptr,{{16,31}},{{16,6}},{{16,16}},{{16,21}},art::{pet}_palette}};')
            lines.append('static const Step '+name+'_steps[]={'+','.join('{&'+name+'_'+str(f)+','+str(clip['frame_ms'])+'}' for f in range(clip['frames']))+'};')
            lines.append(f'static const Clip {name}={{"{name}",{name}_steps,{clip["frames"]},true}};')
            total+=len(indices)
        mapping={s:(m.get('aliases',{}).get(s,s) if s!='pat' else 'celebrate') for s in STATES}
        rows.append('{"'+pet+'",{'+','.join('&'+pet+'_'+mapping[s] for s in STATES)+'}}')
    lines.extend(['const Character characters[]={'+','.join(rows)+'};','const size_t character_count=sizeof(characters)/sizeof(characters[0]);','const Palette palettes[]={{"original",{0}}};','const size_t palette_count=1;','const Accessory accessories[]={{nullptr,nullptr,nullptr,0,{0,0},{0,0},false}};','const size_t accessory_count=0;'])
    scenes={'room':0xF759,'kitchen':0xDF3A,'city':0xAEDB,'night':0x1084}
    for name,color in scenes.items():lines.append(f'static const Scene {name}={{"{name}",{color},nullptr,{{120,192}},4,nullptr,0}};')
    bindings={'eating':'kitchen','traveling':'city','sleepy':'night','ghost':'night'}
    lines.append('const Scene* bindings[11]={'+','.join('&'+bindings.get(s,'room') for s in STATES)+'};')
    lines.append('const size_t pixel_bytes='+str(total+colors)+';\n}')
    (ROOT/'src/generated/assets_data.cpp').write_text('\n'.join(lines)+'\n')
    preview=ROOT/'build/assets';preview.mkdir(parents=True,exist_ok=True)
    sheet=Image.new('RGB',(8*128,((len(frames)+7)//8)*128),'#ddd4be')
    for i,im in enumerate(frames):
        scaled=im.resize((128,128),Image.Resampling.NEAREST);sheet.paste(scaled,(i%8*128,i//8*128),scaled)
    sheet.save(preview/'actual-frames.png')
    print(f'Validated {len(frames)} frames / {len(frames)*1024} pixels and masks; runtime uses {total+colors} pixel/palette bytes (no duplicate mask or RGB arrays).')
if __name__=='__main__':
    import argparse
    ap=argparse.ArgumentParser();ap.add_argument('--export-frames',action='store_true');args=ap.parse_args();build(args.export_frames)
