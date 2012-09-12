import string
import time
import unicodedata
import sys
import struct

output_encode = 'UTF-8'
input_decode = 'UTF-8'
max_chars_per_task = 18
sectors = ""
def get_sector_descriptor(somestr):
	if somestr[0:10] == "SECTORLIST":
		#dlen = "%d"%somestr[11:15]
		dlen = struct.unpack("@I",somestr[10:14])[0]
		print somestr[dlen+15:]
		sys.exit(0)
	return (somestr[14:dlen+14],somestr[dlen+14:])

def encodewrite(somestr):
	global output_encode
	sys.stdout.write(somestr.encode(output_encode))
def print_list(somelist,before,after,toomuch):
	global max_chars_per_task
	islast = False
	j = 0
	bufstr = u""
	for entry in somelist:
		if j == len(somelist)-1:
			islast = True
		if entry != '\n' and len(entry) > 0:
			if len(entry) > max_chars_per_task:
				bufstr+= entry[0:max_chars_per_task]+toomuch
			else:
				bufstr+=entry
			if islast == False:
				bufstr += after
		j+=1
	bufstr+=unicode(time.strftime("%H:%M"))
	bufstr+=u'\n'
	encodewrite(bufstr)

f = open("/tmp/termtask/out",'rb')
hasSec = False
while True:
	content = f.read()
	if len(content) > 0:

		content = content.decode(input_decode)
		if not hasSec:
			(sectors,rest) = get_sector_descriptor(content)
			sectors = unicode.split(sectors,u'\n')
			hasSec = True
		else:
			rest = content
		splitted=unicode.split(rest,u'\4')
		
		for transmission in splitted:
			i = 0
			if len(unicode.strip(transmission)) > 0:
				#print unicode.split(transmission,u'\n')[0:]
				#print_list(unicode.split(transmission,u'\n'),'',u'^fg(#ffff00)^bg(#121415)|^fg()^bg()','...')
				i+=1
	time.sleep(0.4)