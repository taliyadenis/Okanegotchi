"""Semantic catalog/reference validation, not rendering or a general JSON Schema engine."""
import argparse
import copy
import json
from pathlib import Path, PurePosixPath
ROOT=Path(__file__).resolve().parents[1]
STATES=('idle','blink','pat','needs_checkin','sleepy','ghost','eating','traveling','celebrate','neutral','revive')

def require(value,message):
    if not value: raise ValueError(message)

def validate(c):
    require(c['schema_version']==1 and c['asset_version']==1,'catalog version')
    names=('palettes','frames','clips','characters','accessories','backgrounds','scenes')
    tables={}
    for name in names:
        items=c[name]
        require(isinstance(items,list) and len(items)<=256,'registry size')
        table={item['id']:item for item in items}
        require(len(table)==len(items),'duplicate '+name)
        require(all(isinstance(k,str) and k and len(k)<=64 for k in table),'bad ID')
        tables[name]=table
    for p in c['palettes']:
        require(len(p['colors'])==16,'16 palette entries')
        for color in p['colors']:
            require(isinstance(color,str) and len(color)==7 and color[0]=='#','color format')
            int(color[1:],16)
    require(c['import_palette'] in tables['palettes'],'missing canonical import palette')
    opaque=[color.upper() for color in tables['palettes'][c['import_palette']]['colors'][1:]]
    require(len(set(opaque))==15,'ambiguous canonical opaque colors')
    for f in c['frames']:
        w,h=f['size']
        require(type(w)==int and type(h)==int and 8<=w<=64 and 8<=h<=64,'frame dimensions')
        for point in f['anchors'].values():
            require(len(point)==2 and all(type(x)==int for x in point),'anchor coordinates')
            require(0<=point[0]<w and 0<=point[1]<h,'anchor bounds')
        src=f['source']
        require(src['kind'] in ('placeholder','png'),'source kind')
        if src['kind']=='placeholder':
            require(src['shape'] in ('box','diamond','circle') and 1<=src['palette_index']<=15,'placeholder')
        else:
            path=PurePosixPath(src['path'])
            require(not path.is_absolute() and '..' not in path.parts and '\\' not in str(path) and ':' not in str(path),'unsafe source path')
            rect=src['crop']
            require(len(rect)==4 and all(type(x)==int and x>=0 for x in rect) and rect[2:]==[w,h],'crop size')
    for clip in c['clips']:
        require(clip['mode'] in ('loop','once'),'clip mode')
        require(1<=len(clip['frames'])<=32,'clip frame count')
        for step in clip['frames']:
            require(step['frame'] in tables['frames'],'missing frame')
            require(type(step['duration_ms'])==int and 25<=step['duration_ms']<=5000,'frame timing')
    for char in c['characters']:
        require(set(char['animations'])==set(STATES),'semantic state coverage')
        require(char['palettes'] and all(p in tables['palettes'] for p in char['palettes']),'character palettes')
        for ref in char['animations'].values():
            require(ref in tables['clips'],'missing clip')
            for step in tables['clips'][ref]['frames']:
                f=tables['frames'][step['frame']]
                require(f['size']==[32,32] and {'feet','head','neck','body'}<=set(f['anchors']),'character frame geometry')
    for a in c['accessories']:
        require(a['slot'] in ('head','neck','body'),'clothing slot')
        seen=set()
        for v in a['variants']:
            require(v['frame'] in tables['frames'],'accessory frame')
            w,h=tables['frames'][v['frame']]['size']
            require(len(v['origin'])==2 and 0<=v['origin'][0]<w and 0<=v['origin'][1]<h,'accessory origin')
            require(len(v['offset'])==2 and all(type(x)==int and -64<=x<=64 for x in v['offset']),'accessory offset')
            require(v['layer'] in ('behind','front'),'accessory layer')
            for cid in v['characters']:
                require(cid in tables['characters'] and cid not in seen,'variant compatibility/overlap')
                seen.add(cid)
                for ref in tables['characters'][cid]['animations'].values():
                    for step in tables['clips'][ref]['frames']:
                        require(v['anchor'] in tables['frames'][step['frame']]['anchors'],'missing attachment anchor')
    for b in c['backgrounds']:
        require(b['kind'] in ('solid','png'),'background kind')
        if b['kind']=='solid':
            require(len(b['color'])==7 and b['color'][0]=='#','background color'); int(b['color'][1:],16)
        else:
            path=PurePosixPath(b['path'])
            require(not path.is_absolute() and '..' not in path.parts and ':' not in str(path) and '\\' not in str(path),'unsafe background path')
            require(b['size'] in ([60,60],[240,240]),'background dimensions')
    for s in c['scenes']:
        require(s['background'] in tables['backgrounds'],'missing background')
        require(type(s['scale'])==int and 1<=s['scale']<=6,'scene scale')
        require(len(s['character_root'])==2 and all(type(x)==int and 0<=x<240 for x in s['character_root']),'scene root')
        for p in s['props']:
            require(p['frame'] in tables['frames'] and p['layer'] in ('behind','front'),'prop frame/layer')
            require(len(p['screen_xy'])==2 and all(type(x)==int and -240<=x<=480 for x in p['screen_xy']),'prop position')
            require(type(p['scale'])==int and 1<=p['scale']<=6,'prop scale')
    require(set(c['scene_bindings'])==set(STATES),'scene binding coverage')
    require(all(s in tables['scenes'] for s in c['scene_bindings'].values()),'missing scene')

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--catalog',type=Path,default=ROOT/'assets/catalog.example.json')
    ap.add_argument('--self-test',action='store_true')
    args=ap.parse_args()
    catalog=json.loads(args.catalog.read_text(encoding='utf-8'))
    validate(catalog)
    print('PASS example catalog semantic references; PNGs/converter/rendering NOT tested')
    if args.self_test:
        broken=[]
        def changed(fn):
            c=copy.deepcopy(catalog); fn(c); broken.append(c)
        changed(lambda c:c['frames'].append(copy.deepcopy(c['frames'][0])))
        changed(lambda c:c['clips'][0]['frames'][0].update(frame='absent'))
        changed(lambda c:c['clips'][0]['frames'][0].update(duration_ms=0))
        changed(lambda c:c['characters'][0]['animations'].pop('eating'))
        changed(lambda c:c['frames'][0]['anchors'].update(head=[99,99]))
        changed(lambda c:c['accessories'][0]['variants'][0].update(anchor='absent'))
        changed(lambda c:c['scenes'][0].update(background='absent'))
        changed(lambda c:c['scene_bindings'].update(idle='absent'))
        changed(lambda c:c.update(import_palette='absent'))
        changed(lambda c:c['palettes'][0]['colors'].__setitem__(4,c['palettes'][0]['colors'][3]))
        changed(lambda c:c['frames'][0].update(source={'kind':'png','path':'../secret.png','crop':[0,0,32,32]}))
        for c in broken:
            try: validate(c)
            except (ValueError,KeyError,TypeError): continue
            raise AssertionError('invalid catalog accepted')
        print(f'PASS {len(broken)} invalid catalog mutations rejected; NOT firmware validation')

if __name__=='__main__': main()
