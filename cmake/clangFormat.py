import argparse
import sys
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('-i', help="input file", required=True)
parser.add_argument('-f', help="clang-format binary", required=True)
args = vars(parser.parse_args())

filename = args['i']
clangformat = args['f']

def formatContext(file, line):
    return '{file}:{line}:'.format(file=file, line=line)

result = subprocess.run([clangformat, '-style=file', '--output-replacements-xml', filename], stdout=subprocess.PIPE)
outxml = result.stdout.decode('utf-8')

if not 'replacement offset' in outxml:
    sys.exit(0)

replacements = []

import xml.etree.ElementTree as ET

xmlroot = ET.fromstring(outxml)
for elem in xmlroot.findall("./replacement"):
    offset = elem.get('offset')
    length = elem.get('length')
    newText = elem.text
    replacements.append({'offset':int(offset), 'length':int(length), 'newText':newText})

replacements.reverse()
replacement = replacements.pop()

currentLine = 1
currentOffset = 0
alreadyWarnedGlobal = False

with open(filename, "rb") as inFile:
    for line in inFile:
        nextOffset = currentOffset + len(line)
        alreadyWarned = False
        while replacement['offset'] >= currentOffset and replacement['offset'] < nextOffset:
            if not alreadyWarned:
                print('{context} error: format is not conforming to the current style'.format(context=formatContext(filename, currentLine)))
                alreadyWarned = True
                alreadyWarnedGlobal = True
            if len(replacements) == 0:
                sys.exit(1);
            replacement = replacements.pop()
        currentLine = currentLine + 1
        currentOffset = nextOffset

if not alreadyWarnedGlobal:
    print('{context} error: format is not conforming to the current style, but the script failed to point the line (BUG)'.format(context=formatContext(filename, 1)))

sys.exit(1);