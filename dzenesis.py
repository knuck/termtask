import string
import time
import unicodedata
import sys

output_encode = 'UTF-8'
input_decode = 'UTF-8'
max_chars_per_task = 18
def encodewrite(somestr):
	global output_encode
	sys.stdout.write(somestr.encode(output_encode))
def print_list(somelist):
	global max_chars_per_task
	islast = False
	j = 0
	bufstr = u""
	for entry in somelist:
		if j == len(somelist)-1:
			islast = True
		if entry != '\n' and len(entry) > 0:
			if len(entry) > max_chars_per_task:
				bufstr+= entry[0:max_chars_per_task]+u'...'
			else:
				bufstr+=entry
			if islast == False:
				bufstr += u'^fg(#ffff00)^bg(#121415)|^fg()^bg()'
		j+=1
	bufstr+=unicode(time.strftime("%H:%M"))
	bufstr+=u'\n'
	encodewrite(bufstr)

f = open("/tmp/dzenesis",'r')
#content = f.read()
while True:
	content = f.read()
	if len(content) > 0:
		#print content, len(content)
		print content[0:7]
		if content[0:7] == "WINDOWS":
			#print ord(content[12])
			#sys.exit()
			content = content[12:].decode('UTF-8')
			#encodewrite(u"FOUND WINDOWS")
		#splitted = str.split(content,'\4')
		splitted=unicode.split(content,u'\4')
		for transmission in splitted:
			if len(unicode.strip(transmission)) > 0:
				print_list(unicode.split(transmission,u'\n'))
				#print u"----END----"
				#print str.split(slc,'\n').replace('\4','')
		#if splitted != ['\4']:
		#	for slice in content:
		#		print str.split(slice,'\n')
	time.sleep(0.4)