from PIL import Image, ImageFilter
import glob

QUEUE_LENGTH     = 16
DMA_CHUNK_LENGTH = 16
#iso/character/tim files

path = glob.glob('iso/*/*.png')

def recalc(width, height):
    length = width * height
    length = length / 2
    if (((length >= DMA_CHUNK_LENGTH) and (length % DMA_CHUNK_LENGTH))):
        return False
    else:
        return True

def writetex(path):
    print("fixed image " + curpath)
    new_image = Image.new('RGBA', (realwidth, realheight))
    new_image.paste(tex)
    new_image.save(path)

for curpath in path: 
    tex = Image.open(curpath)

    realwidth = tex.size[0]
    realheight = tex.size[1]

    if recalc(realwidth, realheight):
        continue
    print("fixing image " + curpath)

    #try mess with the height
    for i in range(256):
        if realheight > 255
            break;
        if recalc(realwidth, realheight+i):
            realheight = realheight + i;
            break;

    if recalc(realwidth, realheight):
        writetex(curpath)
        continue

    #try mess with the width
    for i in range(256):
        if realwidth > 255
            break;
        if recalc(realwidth+i, realheight):
            realwidth = realwidth + i;
            break;

    if recalc(realwidth, realheight):
        writetex(curpath)
        continue

    print("failed to fix image " + curpath)