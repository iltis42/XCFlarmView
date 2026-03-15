#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import datetime
import csv
import string
import requests
import re

HEADER = """\
/*
 * flarmnet_simple.h - automatisch generiert aus OGN + Flarmnet
 * nur DEVICE_ID, REGISTRATION, CN, TYPE
 */
#ifndef FLARMNET_SIMPLE_H
#define FLARMNET_SIMPLE_H

typedef struct {
    unsigned int id;
    const char *reg;
    const char *cn;
    const char *type;
} flarmnet_entry_t;

static const flarmnet_entry_t flarmnet_db[] = {
"""

FOOTER = """\
};

#endif /* FLARMNET_SIMPLE_H */
"""

URL_OGN = "http://ddb.glidernet.org/download"
URL_FLARMNET = "https://www.flarmnet.org/files/ddb.csv"


# Get current date
today = datetime.date.today()

# Format as DDMMYY
version_str = today.strftime("%d%m%y")

def download_file(url, local_file):
    r = requests.get(url)
    r.raise_for_status()
    with open(local_file, "wb") as f:
        f.write(r.content)
    return local_file

def clean_field(field):
    """Entfernt Quotes, Leerzeichen und prüft Inhalt"""
    return field.strip().strip("'").strip('"')

def clean_ac_type(ac_type):
    """Entfernt Sonderzeichen und kürzt auf 10 Zeichen"""
    max_len = 10

    ac_type = ac_type.replace("Towplane", "Tow")
    ac_type = ac_type.replace("Duo Discus", "DuoDiscus")

    if len(ac_type) > max_len:
        ac_type = ac_type.translate(str.maketrans("", "", string.punctuation)).strip()
    if len(ac_type) > max_len:
        ac_type = ac_type.replace("Discus", "Disc")
        ac_type = ac_type.replace("Ventus", "Vent")
        ac_type = ac_type.replace("Astir", "Ast")
        ac_type = ac_type.replace("Jantar", "Jant")
        ac_type = ac_type.replace("Glasflugel", "Glasfg")
    if len(ac_type) > max_len:
        ac_type = ac_type.replace(" ", "")

    return ac_type[:max_len]

def valid_entry(reg, cn, ac_type):
    """Eintrag nur gültig, wenn REG mindestens 4 alphanumerische Zeichen oder CN nicht leer"""
    
    ac_types_to_ignore = [
        "paraglider",
        "hang",
        "drone",
        "dji",
        "balloon",
        "parachute",
        "motorplane",
        "ultralight",
        "helicopter",
        "gyrocopter",
        "autogyro",
        "ground",
        "ufo",
        "unknown",
        "other",
    ]

    if any(a in ac_type.lower() for a in ac_types_to_ignore):
        return False
    
    if cn:
        return True
    alpha_count = len(re.findall(r'[A-Za-z0-9]', reg))
    return alpha_count >= 4

def load_csv_file(file):
    """Lädt CSV-Datei OGN oder Flarmnet robust"""
    data = {}
    with open(file, "r", encoding="latin1") as f:
        reader = csv.reader(f)
        for row in reader:
            if not row or row[0].startswith("#"):
                continue
            row = [clean_field(c) for c in row]
            if len(row) < 5:
                continue
            fid = row[1]
            ac_type = row[2]
            reg = row[3]
            cn  = row[4]

            if valid_entry(reg, cn, ac_type):
                data[fid] = {"id": fid, "reg": reg, "cn": cn, "type": clean_ac_type(ac_type)}
    return data

def main():
    ogn_file = "iglide_dec.fln"
    flarm_file = "ddb.csv"
    download_file(URL_OGN, ogn_file)
    download_file(URL_FLARMNET, flarm_file)

    ogn_data = load_csv_file(ogn_file)
    flarm_data = load_csv_file(flarm_file)

    # Ergänze Flarmnet nur, wenn ID fehlt
    for fid, entry in flarm_data.items():
        if fid not in ogn_data:
            ogn_data[fid] = entry

    print(f'#define FLARMNET_VERSION "{version_str}"\n')
    print(HEADER)
    sorted_keys = sorted(ogn_data.keys(), key=lambda x: int(x,16))
    for fid in sorted_keys:
        entry = ogn_data[fid]
        print(f'    {{0x{int(fid,16):06X}, "{entry["reg"]}", "{entry["cn"]}", "{entry["type"]}"}},')
    print(FOOTER)

if __name__ == "__main__":
    main()

