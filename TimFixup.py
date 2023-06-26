from PIL import Image, ImageFilter
import glob
import shutil

QUEUE_LENGTH     = 16
DMA_CHUNK_LENGTH = 16
#iso/character/tim files

path = glob.glob('iso/*/*.png')

#calculate if the image is a multiple of 16
def recalc(width, height):
    length = width * height
    length = length / 2
    if ((length >= DMA_CHUNK_LENGTH) and (length % DMA_CHUNK_LENGTH)):
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

    #check if the image needs fixing
    if recalc(realwidth, realheight) == True: 
        continue
    print("fixing image " + curpath)

    #try mess with the height
    for i in range(256):
        if realheight > 255:
            break;
        if recalc(realwidth, realheight+i) == True:
            realheight = realheight + i;
            break;

    # yay fix worked
    if recalc(realwidth, realheight) == True:
        writetex(curpath)
        continue

    #fix didnt work? try mess with the width
    for i in range(256):
        if realwidth > 255:
            break;
        if recalc(realwidth+i, realheight) == True:
            realwidth = realwidth + i;
            break;

    # yay fix worked
    if recalc(realwidth, realheight) == True:
        writetex(curpath)
        continue

    print("failed to fix image " + curpath)