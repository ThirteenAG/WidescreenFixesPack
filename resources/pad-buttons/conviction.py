# Embed controller artwork in the native shared image-substitution movie.
from pathlib import Path
import argparse,hashlib,struct,zlib
from PIL import Image
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('original',type=Path);parser.add_argument('output',type=Path);args=parser.parse_args()
source=args.original.read_bytes()
if hashlib.sha256(source).hexdigest()!='cc0d749aca41c693eb77c0a8fb87cd7648ec1bf692e7390e846d158970b40104':
 raise SystemExit('Unsupported original image-substitution movie')
assert source[:3]==b'GFX'
header=8+(5+4*(source[8]>>3)+7)//8+4
cells={1:0,2:1,3:2,4:3,5:4,6:5,7:10,8:11,9:8,10:9,11:7,12:6,13:12,14:15,15:13,16:14}
args.output.mkdir(parents=True,exist_ok=True)
for family in ('playstation','nintendo'):
 atlas=Image.open(root/'textures/RWGTA/ButtonIcons'/('pad_'+family+'.png')).convert('RGBA')
 position=header;tags=[];replaced=[]
 while position<len(source):
  start=position;record=struct.unpack_from('<H',source,position)[0];position+=2;kind=record>>6;size=record&63
  if size==63:size=struct.unpack_from('<I',source,position)[0];position+=4
  payload=source[position:position+size];position+=size;assert position<=len(source)
  if kind==1001:
   name=payload[9:9+payload[8]].decode('ascii')
   if name.startswith('IK_JOY') and name[6:].isdigit() and int(name[6:]) in cells:
    button=int(name[6:]);cell=cells[button];width,height=struct.unpack_from('<HH',payload,4)
    image=atlas.crop(((cell%8)*64,(cell//8)*64,(cell%8+1)*64,(cell//8+1)*64)).resize((width,height),Image.Resampling.LANCZOS)
    argb=bytearray()
    pixels=image.tobytes()
    for at in range(0,len(pixels),4):
     r,g,b,a=pixels[at:at+4];argb.extend((a,(r*a+127)//255,(g*a+127)//255,(b*a+127)//255))
    payload=struct.pack('<HBHH',struct.unpack_from('<H',payload)[0],5,width,height)+zlib.compress(argb,9)
    tags.append(struct.pack('<HI',(36<<6)|63,len(payload))+payload);replaced.append(button);continue
  tags.append(source[start:position])
  if kind==0:break
 assert sorted(replaced)==list(range(1,17))
 result=bytearray(source[:header]+b''.join(tags));struct.pack_into('<I',result,4,len(result))
 target=args.output/('buttons-'+family+'.gfx');target.write_bytes(result);print(target.name,len(result),hashlib.sha256(result).hexdigest())
