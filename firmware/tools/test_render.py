"""Compare the actual C++ tile compositor to source sheets, including 4x placement."""
from PIL import Image,ImageDraw
import json
from pathlib import Path
from test_runtime import Driver,ROOT
def rgb565(p):return ((p[0]>>3)<<11)|((p[1]>>2)<<5)|(p[2]>>3)
def main():
    d=Driver();m=json.loads((ROOT.parent/'assets/manifest.json').read_text(encoding='utf-8'));tested=0
    board=Image.new('RGB',(4*240,3*264),'#182328');label=ImageDraw.Draw(board)
    try:
        for row,pet in enumerate(m['presets']):
            for state,clip in m['animations'].items():
                sheet=Image.open(ROOT.parent/'assets/source'/pet/clip['file']).convert('RGBA')
                background=0xDF3A if state=='eating' else 0xAEDB if state=='traveling' else 0x1084 if state in ('sleepy','ghost') else 0xF759
                for f in range(clip['frames']):
                    result=d.call('render',pet=pet,animation=state,ms=f*clip['frame_ms'],pixels=True)
                    raw=bytes.fromhex(''.join(result['pixels']));actual=[int.from_bytes(raw[i:i+2],'big') for i in range(0,len(raw),2)]
                    expected=[background]*(240*240);image=sheet.crop((f*32,0,f*32+32,32))
                    for n,p in enumerate(image.getdata()):
                        if not p[3]:continue
                        x=56+(n%32)*4;y=68+(n//32)*4;color=rgb565(p)
                        for dy in range(4):expected[(y+dy)*240+x:(y+dy)*240+x+4]=[color]*4
                    if actual!=expected:raise AssertionError(f'{pet}/{state}/{f}: compositor differs from source or placement')
                    tested+=1
                    if f==0 and state in ('idle','eating','traveling','ghost'):
                        col=('idle','eating','traveling','ghost').index(state);im=Image.new('RGB',(240,240));im.putdata([(((c>>11)&31)*255//31,((c>>5)&63)*255//63,(c&31)*255//31) for c in actual]);board.paste(im,(col*240,row*264));label.text((col*240+8,row*264+244),pet+' / '+state,fill='white')
        dest=ROOT/'build/assets/runtime-preview.png';dest.parent.mkdir(parents=True,exist_ok=True);board.save(dest)
        print(f'PASS {tested} rendered frames, {tested*240*240:,} RGB565 pixel comparisons. Preview: {dest}')
    finally:d.close()
if __name__=='__main__':main()
