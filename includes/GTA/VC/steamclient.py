import shutil
import glob

class SteamClientGenerator:
	def __init__(self):
		self.normal = [
			0x401000, 0x41CBCE, 0x41CC07, 0x41CC1F, 0x41CC20, 0x434720, 0x434730, 0x434FE6,
			0x435029, 0x435033, 0x435040, 0x4434B0, 0x443E00, 0x45189F, 0x4518D3, 0x4518D7,
			0x4518E0, 0x489D0A, 0x489D10, 0x489D82, 0x489D82, 0x489E0D, 0x489E10, 0x48E6E7,
			0x48E6FD, 0x48E715, 0x48E830, 0x48E90B, 0x48EC30, 0x498FF3, 0x498FF3, 0x49B97C,
			0x49B993, 0x49BB4D, 0x49BB91, 0x49BCF0, 0x49BD00, 0x4A4352, 0x4A4360, 0x4A443D,
			0x4A448A, 0x4A4586, 0x4A4586, 0x4A4668, 0x4A4670, 0x4A4732, 0x4A4734, 0x4A47AA,
			0x4A47B0, 0x4A4BE3, 0x4A4BE3, 0x4A4CF3, 0x4A4CF3, 0x4A4FD0, 0x4A4FD0, 0x4C4220,
			0x4C4230, 0x516F87, 0x516F87, 0x51705A, 0x517060, 0x57BE80, 0x57BF40, 0x5CC730,
			0x5CC921, 0x5CC973, 0x5CC9E0, 0x5D71C0, 0x5D7270, 0x5D7940, 0x5D7A12, 0x5D7A3C,
			0x5D7A40, 0x600FB0, 0x600FB0, 0x601E80, 0x602000, 0x61A700, 0x61D3D0, 0x62121B,
			0x62121B, 0x621222, 0x621230, 0x626F70, 0x626F80, 0x6271ED, 0x62721B, 0x6272DB,
			0x6272E0, 0x6273B0, 0x6274D0, 0x627570, 0x627570, 0x62758D, 0x627590, 0x637608,
			0x63762B, 0x6378C2, 0x639268, 0x67DD06, 0x67E000, 0x68008C, 0x683000, 0x68DD7C,
			0x68DD7C, 0x69A014, 0x69A020, 0x69A0C8, 0x69A0C8, 0x6A557C, 0x6A5580, 0x6A573C,
			0x6A5740, 0x6D665C, 0x6D668C, 0x6D6B28, 0x6D6B28, 0x6DB8E8, 0x6DB8F0, 0x6DB924,
			0x6DB924, 0x6DC9F8, 0x6DCAE0, 0x6DF9F8, 0x6F23C0, 0x6F2430, 0x6F2434, 0x6F2458,
			0x6F245C, 0x6F26A0, 0x6F7000, 0x786BA8, 0x786BA8, 0x786D40, 0x786D40, 0xA10B20,
			0xA10B20, 0xA10B4E, 0xA10B4E, 0xA10BA1
		]
		
		self.steam = [
			0x401000, 0x41CBCE, 0x41CBD0, 0x41CBE8, 0x41CBF0, 0x4346F0, 0x4346F0, 0x434FA6,
			0x434FA6, 0x434FB0, 0x434FB0, 0x443420, 0x443D10, 0x4517AF, 0x4517AF, 0x4517B3,
			0x4517C0, 0x489BEA, 0x489BFD, 0x489C6F, 0x489C70, 0x489CFB, 0x489D00, 0x48E5D7,
			0x48E5EF, 0x48E607, 0x48E73E, 0x48E819, 0x48EB40, 0x498F03, 0x498F14, 0x49B89D,
			0x49B89D, 0x49BA57, 0x49BA60, 0x49BBBF, 0x49BBBF, 0x4A4211, 0x4A4220, 0x4A42FD,
			0x4A42FD, 0x4A43F9, 0x4A4400, 0x4A44E2, 0x4A44F0, 0x4A45B2, 0x4A45F0, 0x4A4666,
			0x4A4670, 0x4A4AA3, 0x4A4AAA, 0x4A4BBA, 0x4A4BC0, 0x4A4E9D, 0x4A4EA0, 0x4C40F0,
			0x4C40F0, 0x516E47, 0x516E76, 0x516F49, 0x516F50, 0x57BD70, 0x57BD70, 0x5CC560,
			0x5CC6FC, 0x5CC74E, 0x5CC7B0, 0x5D6F90, 0x5D6FA0, 0x5D7670, 0x5D7670, 0x5D769A,
			0x5D76A0, 0x600C10, 0x600C20, 0x601AF0, 0x601C20, 0x61A320, 0x61D010, 0x620E5B,
			0x620E6C, 0x620E73, 0x620E80, 0x626BC0, 0x626BD0, 0x626E3D, 0x626E9A, 0x626F5A,
			0x626F60, 0x627030, 0x627160, 0x627200, 0x627226, 0x627243, 0x627250, 0x6372C8,
			0x6372EA, 0x637581, 0x638218, 0x67CCB6, 0x67D000, 0x67F08C, 0x682000, 0x68CD7C,
			0x68CD84, 0x69901C, 0x69901C, 0x6990C4, 0x6990C8, 0x6A457C, 0x6A457C, 0x6A4738,
			0x6A4738, 0x6D5654, 0x6D5658, 0x6D5AF4, 0x6D5AF8, 0x6DA8B8, 0x6DA8B8, 0x6DA8EC,
			0x6DA90C, 0x6DB9E0, 0x6DBA78, 0x6DE990, 0x6F13B8, 0x6F1428, 0x6F1428, 0x6F144C,
			0x6F144C, 0x6F1690, 0x6F6000, 0x785BA8, 0x785BAC, 0x785D44, 0x785D48, 0xA0FB28,
			0xA0FB29, 0xA0FB57, 0xA0FB58, 0xA0FBAB
		]
		
		self.codeReplaced = 0
		self.dataReplaced = 0
		self.failedCode = 0
		self.failedData = 0
		self.failedCodeSet = set()
		self.failedDataSet = set()
		self.patchSuccess = 0
		self.patchFailed = 0
		self.logFile = None
		
	def openLog(self):
		self.logFile = open("convertlog.txt", "w")
		
	def closeLog(self):
		if self.logFile:
			self.logFile.close()
			self.logFile = None
		
	def logInfo(self, log):
		print(log)
		
		if self.logFile:
			self.logFile.write(log + "\n")
		
	def searchListForFrame(self, l, n):
		s = 0
		e = len(l) - 1

		while True:
			x = (s + e) // 2

			if s == e:
				return s
			elif l[x] > n:
				e = x - 1
			elif l[x+1] <= n:
				s = x + 1
			else:
				return x
				
	def findValueReplace(self, address, mayTouch = False, reverse = False):
		sourceList = self.normal if (reverse == False) else self.steam
		targetList = self.normal if (reverse == True) else self.steam
	
		index = self.searchListForFrame(sourceList, address)
		isData = (address >= 0x67DD06)

		if (index % 2 == 1) and not (mayTouch and sourceList[index] == address):
			if isData:
				self.failedData += 1
				self.failedDataSet.add(address)
				
				self.logInfo("Data address %06Xh is in a hole %d, val %06Xh." % (address, index, sourceList[index]))
			else:
				self.failedCode += 1
				self.failedCodeSet.add(address)
				
				self.logInfo("Code address %06Xh is in a hole %d, val %06Xh." % (address, index, sourceList[index]))
			
			return address
			
		if isData:
			self.dataReplaced += 1
		else:
			self.codeReplaced += 1

		return address - sourceList[index] + targetList[index]
		
	def getNumStringReplacement(self, numstring, prepend, mayTouch = False, reverse = False):
		val = int(numstring, 16)

		if val >= 0x401000 and val < 0xA12000:
			rep = self.findValueReplace(val, mayTouch)
			
			if prepend and rep >= 0xA00000:
				return "%07X" % rep
			else:
				return "%06X" % rep
		else:
			return numstring
			

	def x(self, numstring):
		return self.getNumStringReplacement(numstring, True, False)

	def y(self, numstring):
		return self.getNumStringReplacement(numstring, True, True)
		
	def getDirSources(self, dirPath):
		return glob.glob(dirPath + "/*.cpp") + glob.glob(dirPath + "/*.h")
		
	def generate(self, sourcePath, targetPath, convertedDirs):
		self.openLog()
	
		shutil.rmtree(targetPath, True)
		shutil.copytree(sourcePath, targetPath, ignore = shutil.ignore_patterns("deploy.py", "*.pyc", "__pycache__", "Release\\*", "Debug\\*", "*.opensdf", "*.sdf"))
		
		fileList = []
		
		for dir in convertedDirs:
			fileList += self.getDirSources(targetPath + "\\" + dir)
			
		for file in fileList:
			self.logInfo("Processing file " + file + ".")
		
			self.processFile(file, targetPath)
			
		self.logInfo("Successfully replaced " + str(self.codeReplaced) + " code and " + str(self.dataReplaced) + " data addresses.")
		
		if self.failedCode > 0:
			self.logInfo("Failed to replace " + str(self.failedCode) + " (" + str(len(self.failedCodeSet)) + " unique) code addresses.")
			
		if self.failedData > 0:
			self.logInfo("Failed to replace " + str(self.failedData) + " (" + str(len(self.failedDataSet)) + " unique) data addresses.")
			
		if self.patchSuccess > 0:
			self.logInfo("Successfully applied " + str(self.patchSuccess) + " manual patches.")
			
		if self.patchFailed > 0:
			self.logInfo("Failed to apply " + str(self.patchFailed) + " manual patches.")
			
		self.closeLog()
			
	def processFile(self, path, targetBase):
		writeData, readPart, readStatus, mayTouch = "", "", 0, False
	
		with open(path, "r") as fi:
			inputData = fi.read()
			
			inputData = self.doPatches(False, path, inputData, targetBase)
			
			for char in inputData:
				if readStatus == 0:
					if char == "0":
						readStatus = 1
						mayTouch = (writeData[-7:] == "ASMJMP(") or (writeData[-8:] == "EAXJUMP(") or (writeData[-8:] == "EAXCALL(")
						readPart += char
					elif char in "0123456789ABCDEFabcdef":
						mayTouch = (writeData[-7:] == "ASMJMP(") or (writeData[-8:] == "EAXJUMP(") or (writeData[-8:] == "EAXCALL(")
						readStatus = 3
						readPart += char
					else:
						writeData += char
						
				elif readStatus == 1:
					if char == "x":
						readStatus = 2
						readPart += char
					elif char in "0123456789ABCDEFabcdef":
						readPart += char
						readStatus = 3
					else:
						writeData += readPart + char
						readPart, readStatus = "", 0
				
				elif readStatus == 2:
					if char in "0123456789ABCDEFabcdef":
						readPart += char
					else:
						if len(readPart) == 2:
							writeData += readPart
						else:
							writeData += "0x" + self.getNumStringReplacement(readPart[2:], False, mayTouch)

						writeData += char
						readPart, readStatus = "", 0

				elif readStatus == 3:
					if char in "0123456789ABCDEFabcdef":
						readPart += char
					else:
						if char == "h":
							writeData += self.getNumStringReplacement(readPart, True, mayTouch)
						else:
							writeData += readPart

						writeData += char
						readPart, readStatus = "", 0
						
			writeData += readPart
			
			fi.close()
			
			writeData = self.doPatches(True, path, writeData, targetBase)
			
			with open(path, "w") as fo:
				fo.write(writeData)
				fo.close()
		
	def doPatches(self, isFinal, file, data, targetBase):
		patches = glob.glob("steampatches\\*.txt")
		changed = data
		
		for patchfile in patches:
			with open(patchfile) as f:
				pieces = [x.strip() for x in f.read().split("||||")]
				
				if file == targetBase + "\\" + pieces[0]:
					if isFinal:
						newChanged = changed.replace("{!" + patchfile + "}", pieces[2])
					else:
						newChanged = changed.replace(pieces[1], "{!" + patchfile + "}")
					
					if isFinal:
						if newChanged == changed:
							self.logInfo("Failed to apply patch from " + patchfile + ".")
							self.patchFailed += 1
						else:
							self.logInfo("Patch from " + patchfile + " successful.")
							self.patchSuccess += 1
						
					changed = newChanged
				
				f.close()
				
		return changed

