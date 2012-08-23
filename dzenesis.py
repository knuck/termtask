import string
import time
import unicodedata

def print_list(somelist):
	for entry in somelist:
		if entry != '\n' and len(entry) > 0:
			if len(entry) > 22:
				print len(entry), entry[0:22]+u'...'
			else:
				print len(entry), entry

f = open("/tmp/dzenesis",'r')
#content = f.read()
while True:
	content = f.read().decode('UTF-8')
	if len(content) > 0:
		#print content, len(content)
		
		#splitted = str.split(content,'\4')
		splitted=unicode.split(content,u'\4')
		for transmission in splitted:
			if len(unicode.strip(transmission)) > 0:
				print_list(unicode.split(transmission,'\n'))
				print u"----END----"
				#print str.split(slc,'\n').replace('\4','')
		#if splitted != ['\4']:
		#	for slice in content:
		#		print str.split(slice,'\n')
		#time.sleep(0.4)