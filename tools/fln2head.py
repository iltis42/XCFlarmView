import xml.etree.ElementTree as ET
import sys
import requests

URL = "https://www.flarmnet.org/static/files/fln/iglide_dec.fln"
URLGN = "http://ddb.glidernet.org/download"
# 2. download the data behind the URL
response = requests.get(URL)
responseGN = requests.get(URLGN)
# 3. Open the response into a new file called instagram.ico
open("iglide_dec.fln", "wb").write(response.content)
open("glidernet.fln", "wb").write(responseGN.content)

tree = ET.parse("iglide_dec.fln")
root = tree.getroot()


print("#include \"flarmnet.h\"")
print("#define FLARMNET_VERSION \"" + root.get('Version') + "\"")
print("t_flarmnet flarmnet[] = {")

flarmnet = {}

for plane in root.findall('FLARMDATA'):
    reg = plane.find('REG').text
    if reg:
        reg=reg.replace(" ", "")
    cid = plane.find('COMPID').text
    if cid:
        cid=cid.replace(" ", "")
    flarmid = plane.get('FlarmID')
    if not reg or reg == '0000':
        reg=""
    if not cid:
        cid=""
    if reg != "":
        flarmnet[flarmid] = [ reg, cid ]

file1 = open('glidernet.fln', 'r')
Lines = file1.readlines()

num=0
for line in Lines:
    num+=1
    if( num > 1 ):
       plane = line.split(',')
       flarmid = plane[1].strip("'")
       reg = plane[3].strip("'")
       cid = plane[4].strip("'")
       if len( reg ) or len( cid ):
           flarmnet[flarmid] = [ reg, cid ]
        


for id in flarmnet:
    print("{ 0x"+ id +",\""+ flarmnet[id][0][:6]+"\",\""+ flarmnet[id][1][:3]+"\"}," )




print("};")

