#!/usr/bin/env python

import datetime

NUMS = '''01 02 04 05 06 08 09
	10 11 12 13 15 16 17 18 19
	20 21 22 23 24 25 26 27 28 29
	30 31 32 33 34 35 36 37 38 39
	40 41 42 44 45 46 47 48 49
	50 51 53 54 55 56'''

CODES = '''al ak az ar ca co ct
	de dc fl ga hi id il in ia
	ks ky la me md ma mi mn ms mo
	mt ne nv nh nj nm ny nc nd oh
	ok or pa ri sc sd tn tx ut
	vt va wa wv wi wy'''

USHR2010 = '''7 1 9 4 53 7 5
	1 0 27 14 2 2 18 9 4
	4 6 6 2 8 9 14 8 4 8
	1 3 4 2 12 3 27 13 1 16
	5 5 18 2 7 1 9 36 4
	1 11 10 3 8 1'''

SLH2010 = '''105 40 60 100 80 65 151
	41 8 120 180 51 70 118 100 100
	125 100 105 151 141 160 110 134 122 163
	100 49 42 400 80 70 150 120 94 99
	101 60 203 75 124 70 99 150 75
	150 100 98 100 99 60'''

nums = NUMS.split()
codes = CODES.split()
districts = {
	'ushr2010': USHR2010.split(),
	'slh2016': SLH2010.split()}
n = len(nums)
dbfs = [codes[i]+'/'+'tabblock2010_'+nums[i]+'_pophu.dbf' for i in range(n)]
smos = [codes[i]+'/'+codes[i]+'_ushr2010.smo' for i in range(n)]
smgs = [codes[i]+'/'+codes[i]+'.smg' for i in range(n)]
shps = [codes[i]+'/tabblock2010_'+nums[i]+'_pophu.shp' for i in range(n)]
zips = ['downloads/tabblock2010_'+nums[i]+'_pophu.zip' for i in range(n)]

mf = open('Makefile', 'w')
mf.write('''

SSM = ../ssm/ssm
SHP2SMG = ../shp2smg/shp2smg
SMOLOD = ../smolod/smolod
DOWNLOAD = wget
WEBFOLDER = http://www2.census.gov/geo/tiger/TIGER2010BLKPOPHU

''')

mf.write('CODES = ' + ' '.join(codes) + '\n')
mf.write('DBFS = ' + ' '.join(dbfs) + '\n')
mf.write('SMOS = ' + ' '.join(smos) + '\n')
mf.write('SMGS = ' + ' '.join(smgs) + '\n')
mf.write('SHPS = ' + ' '.join(shps) + '\n')
mf.write('ZIPS = ' + ' '.join(zips) + '\n')

mf.write('''
.PHONY: codes dbfs smos smgs shps zips make_ssm make_shp2smg make_smolod
codes: ushr2010
dbfs: $(DBFS)
smos: $(SMOS)
smgs: $(SMGS)
shps: $(SHPS)
zips: $(ZIPS)

make_ssm:
	$(MAKE) -C ../ssm

make_shp2smg:
	$(MAKE) -C ../shp2smg

make_smolod:
	$(MAKE) -C ../smolod

$(SSM): | make_ssm ;
$(SHP2SMG): | make_shp2smg ;
$(SMOLOD): | make_smolod ;

''')

mf.write('.PHONY: '+' '.join(districts.keys())+'\n')
for name, numd in districts.items():
	mf.write(name+': '+' '.join([codes[i]+'_'+name for i in range(n) if int(numd[i]) > 1])+'\n')
mf.write('\n')

for i in range(n):
	mf.write(zips[i]+':\n\tmkdir -p downloads/\n\twget -P downloads/ $(WEBFOLDER)/tabblock2010_'+nums[i]+'_pophu.zip\n')
mf.write('\n')

for i in range(n):
	mf.write(shps[i]+': '+zips[i]+'\n\tunzip -n $< -d '+codes[i]+'\n')
mf.write('\n')

for i in range(n):
	mf.write(smgs[i]+': $(SHP2SMG) '+shps[i]+'\n\t$(SHP2SMG) '+shps[i]+' > '+codes[i]+'/'+codes[i]+'.smg\n')
mf.write('\n')

dt = datetime.datetime.now().isoformat()
for name, numd in districts.items():
	for i in range(n):
		if int(numd[i]) > 1:
			key = codes[i] + '_' + name
			smo = codes[i] + '/' + key + '.smo'
			mf.write(smo+': $(SSM) '+smgs[i]+'\n\t$(SSM) divide '+numd[i]+' < '+smgs[i]+' > '+smo+'\n')
			mf.write(key+': $(SMOLOD) '+smo+'\n\t$(SMOLOD) '+smo+' '+codes[i]+'/tabblock2010_'+nums[i]+'_pophu.dbf ssm_'+name+'_'+dt+'\n')
	mf.write('\n')

"""
for i in range(n):
	mf.write(codes[i]+': '+dbfs[i]+'\n')
mf.write('\n')
"""

mf.write('\n')

mf.close()


