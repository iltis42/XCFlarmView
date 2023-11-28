import xml.etree.ElementTree as ET
import sys
import requests

URL = "https://www.flarmnet.org/static/files/fln/iglide_dec.fln"
# 2. download the data behind the URL
response = requests.get(URL)
# 3. Open the response into a new file called instagram.ico
open("iglide_dec.fln", "wb").write(response.content)

tree = ET.parse("iglide_dec.fln")
root = tree.getroot()

print("#include \"flarmnet.h\"")
print("t_flarmnet flarmnet[] = {")


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
       print("{ 0x"+ flarmid +",\""+reg[:6]+"\",\""+ cid[:3]+"\"}," )
print("};")

