"""Generate placeholder metadata and expected animation samples; no final artwork."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
STATES=['idle','blink','pat','needs_checkin','sleepy','ghost','eating','traveling','celebrate','neutral','revive']
def save(path,obj):
    p=ROOT/path; p.parent.mkdir(parents=True,exist_ok=True)
    p.write_text(json.dumps(obj,indent=2)+'\n',encoding='utf-8')
def object_schema(properties,required=None):
    return {'type':'object','properties':properties,'required':list(properties) if required is None else required,'additionalProperties':False}
def array(items,maximum=256,minimum=0):
    return {'type':'array','items':items,'minItems':minimum,'maxItems':maximum}
def pair(low,high):
    return {'type':'array','items':{'type':'integer','minimum':low,'maximum':high},'minItems':2,'maxItems':2}
def main():
    colors=['#000000','#FFFFFF','#222222','#50BFA0','#F48A8A','#9370DB','#F5D56A','#5C9BD5',
            '#B47850','#F5E8CA','#699E55','#9B536C','#D5D5D5','#683F2E','#D28742','#2C4C63']
    catalog={'schema_version':1,'asset_version':1,'import_palette':'mint','palettes':[], 'frames':[], 'clips':[],
             'characters':[],'accessories':[],'backgrounds':[],'scenes':[],'scene_bindings':{}}
    for name,color in [('mint','#50BFA0'),('coral','#F48A8A'),('lavender','#9370DB')]:
        p=colors.copy();p[3]=color;catalog['palettes'].append({'id':name,'colors':p})
    for pet,shape in [('piggy','box'),('cat','diamond'),('dragon','circle')]:
        for i in range(2):
            catalog['frames'].append({'id':f'{pet}.{i}','size':[32,32],
                'source':{'kind':'placeholder','shape':shape,'palette_index':3},
                'anchors':{'feet':[16,30],'head':[16,6-i],'neck':[16,16-i],'body':[16,21-i]}})
        for clip,timing in [('idle',500),('eating',125),('traveling',200)]:
            catalog['clips'].append({'id':f'{pet}.{clip}','mode':'loop',
                'frames':[{'frame':f'{pet}.{i%2}','duration_ms':timing} for i in range(4)]})
        catalog['characters'].append({'id':pet,'palettes':['mint','coral','lavender'],
            'animations':{s:f'{pet}.{s if s in ("eating","traveling") else "idle"}' for s in STATES}})
    for item,color in [('cap',6),('scarf',4),('food-prop',8)]:
        catalog['frames'].append({'id':item+'.frame','size':[16,16],
            'source':{'kind':'placeholder','shape':'box','palette_index':color},'anchors':{}})
    for name,anchor,origin in [('cap','head',[8,12]),('scarf','neck',[8,4])]:
        catalog['accessories'].append({'id':name,'slot':anchor,'variants':[{
            'characters':['piggy','cat','dragon'],'frame':name+'.frame','anchor':anchor,
            'origin':origin,'offset':[0,0],'layer':'front'}]})
    for name,color in [('room','#F5E8CA'),('kitchen','#D5D5D5'),('city','#5C9BD5'),('night','#222222')]:
        catalog['backgrounds'].append({'id':name,'kind':'solid','color':color})
        catalog['scenes'].append({'id':name,'background':name,'character_root':[120,190],'scale':4,
            'props':[{'frame':'food-prop.frame','screen_xy':[166,158],'scale':2,'layer':'front'}] if name=='kitchen' else []})
    catalog['scene_bindings']={s:('kitchen' if s=='eating' else 'city' if s=='traveling' else 'night' if s=='ghost' else 'room') for s in STATES}
    save('assets/catalog.example.json',catalog)
    ident={'type':'string','pattern':'^[a-z][a-z0-9._-]{0,63}$'}
    color={'type':'string','pattern':'^#[0-9A-Fa-f]{6}$'}
    integer={'type':'integer','minimum':1,'maximum':6}
    source={'oneOf':[
        object_schema({'kind':{'const':'placeholder'},'shape':{'enum':['box','diamond','circle']},'palette_index':{'type':'integer','minimum':1,'maximum':15}}),
        object_schema({'kind':{'const':'png'},'path':{'type':'string','minLength':1,'maxLength':240},
            'crop':{'type':'array','items':{'type':'integer','minimum':0},'minItems':4,'maxItems':4}})]}
    frame=object_schema({'id':ident,'size':pair(8,64),'source':source,
        'anchors':{'type':'object','propertyNames':ident,'additionalProperties':pair(0,63)}})
    clip=object_schema({'id':ident,'mode':{'enum':['loop','once']},'frames':array(object_schema({
        'frame':ident,'duration_ms':{'type':'integer','minimum':25,'maximum':5000}}),32,1)})
    character=object_schema({'id':ident,'palettes':array(ident,16,1), 'animations':object_schema({s:ident for s in STATES})})
    variant=object_schema({'characters':array(ident,64,1),'frame':ident,'anchor':ident,'origin':pair(0,63),'offset':pair(-64,64),'layer':{'enum':['behind','front']}})
    accessory=object_schema({'id':ident,'slot':{'enum':['head','neck','body']},'variants':array(variant,64,1)})
    background={'oneOf':[object_schema({'id':ident,'kind':{'const':'solid'},'color':color}),
        object_schema({'id':ident,'kind':{'const':'png'},'path':{'type':'string','minLength':1,'maxLength':240},'size':{'enum':[[60,60],[240,240]]}})]}
    prop=object_schema({'frame':ident,'screen_xy':pair(-240,480),'scale':integer,'layer':{'enum':['behind','front']}})
    scene=object_schema({'id':ident,'background':ident,'character_root':pair(0,239),'scale':integer,'props':array(prop,8)})
    schema=object_schema({'schema_version':{'const':1},'asset_version':{'const':1},'import_palette':ident,
        'palettes':array(object_schema({'id':ident,'colors':array(color,16,16)}),16,1),
        'frames':array(frame,256,1),'clips':array(clip,256,1),'characters':array(character,64,1),
        'accessories':array(accessory,64),'backgrounds':array(background,64,1),'scenes':array(scene,64,1),
        'scene_bindings':object_schema({s:ident for s in STATES})})
    schema.update({'$schema':'https://json-schema.org/draft/2020-12/schema','title':'Okanegachi compiled asset catalog v1'})
    save('assets/catalog.schema.json',schema)
    samples=[]
    for mode in ('loop','once'):
        for t in (0,124,125,249,250,374,375,499,500,7999,8000):
            phase=t%500 if mode=='loop' else min(t,499)
            samples.append({'id':f'{mode}-{t}','durations_ms':[125]*4,'mode':mode,'elapsed_ms':t,'expected_frame':phase//125})
    save('tests/animation/samples.json',samples)
    print('Generated placeholder catalog/schema and22 animation samples; no artwork or firmware')
if __name__=='__main__':main()
