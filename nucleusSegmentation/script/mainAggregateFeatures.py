#! /usr/bin/env python

import os.path
import glob
import sys, getopt
import socket
import numpy as np

def readDictionary(fileName):
    try:
        f = open(fileName, 'r')
    except IOError:
        print 'cannot open', fileName
        sys.exit(2) 
    readContent = f.read().split("\n")
    readContent.pop(0)
    featureDict = {}
    for i in range(len(readContent)-1):
        nameType = readContent[i].split(",")
        nameType[0] = nameType[0].strip(" \n\r\t")
        if (nameType[0]!=""):
           nameType[1] = nameType[1].strip(" \n\t\r")
           featureDict[nameType[0]] = nameType[1]
    f.close()
    return (featureDict)

def readExcludeFile(excludeFile):
    if (excludeFile==''):
        return set([]) 
    try:
        f = open(excludeFile,'r')
    except IOError:
        print 'cannot open', fileName
        sys.exit(2) 
    readContent = f.read().split("\n")
    excludeList = set([]) 
    for i in range(len(readContent)-1):
        excludeFeatures = readContent[i].split(",")
        for j in range(len(excludeFeatures)):
            excludeFeatures[j] = excludeFeatures[j].strip(" \n\t\r")
            excludeList.add(excludeFeatures[j])
    return excludeList

def checkDictionary(featureDict,featureList):
    check = 1
    for i in range(len(featureList)):
        if (featureList[i] not in featureDict):
           print 'ERROR: feature:',featureList[i],' is not in dictionary.'
           check = 0
    return (check)

def readCSV(dataDir,featureDict):
    filePathnameList = glob.glob(dataDir) 
    allContent  = [] 
    numFeatures = 0
    featureList = []
    fileCnt = 0
    for j in filePathnameList: 
       print "Reading: ",j
       f = open(j,'r')
       readContent = f.read().split('\n')
       fileCnt += 1
       if (fileCnt==1):
          featureList = readContent[0].split(',')
          numFeatures = len(featureList)
          if (featureList[numFeatures-1]=='' or featureList[numFeatures-1]==' '):
             numFeatures = numFeatures-1
          for i in range(numFeatures):
              featureList[i] = featureList[i].strip(" \t\n\r")
          check = checkDictionary(featureDict,featureList)
          if (check==0):
             print "ERROR in file:",j
             sys.exit(2)
       else:
          if (numFeatures!=len(readContent[0].split(','))):
             print 'ERROR: number of features in different files does not match.'
             print 'number of features: ', numFeatures, ' is not equal to ', len(readContent[0].split(','))
             sys.exit(2)
       readContent.pop(0)
       allContent += readContent 
       mydata = allContent.pop()
       f.close()   
    return (allContent,featureList,numFeatures)

def writeHeaderCSV(featureNames,excludeList,numberOfFeatures,fout): 
    fout.write("ParticipantBarcode")
    fout.write(",")
    fout.write("ImagingModality")
    fout.write(",")
    for j in range(numberOfFeatures):
        if (featureNames[j] not in excludeList):
           fout.write(str(featureNames[j]) + "_Q25")
           fout.write(",")
    for j in range(numberOfFeatures):
        if (featureNames[j] not in excludeList):
           fout.write(str(featureNames[j]) + "_median")
           fout.write(",")
    for j in range(numberOfFeatures):
        if (featureNames[j] not in excludeList):
           fout.write(str(featureNames[j]) + "_Q75")
           fout.write(",")
    fout.write("SampleTypeLetterCode,")
    fout.write("Study,")
    fout.write("AnalysisId\n")

def writeDataCSV(featureNames,excludeList,patientId,subjectFeatures,numberOfFeatures,studyId,letterCode,analysisId,fout):
    writeHeaderCSV(featureNames,excludeList,numberOfFeatures,fout)
    fout.write(str(patientId))
    fout.write(",")
    fout.write("pathology")
    fout.write(",")
    i = 0
    for j in range(numberOfFeatures):
        if (featureNames[j] not in excludeList):
           fout.write(str(subjectFeatures[i]))
           fout.write(",")
        i = i + 1 
    for j in range(numberOfFeatures):
        if (featureNames[j] not in excludeList):
           fout.write(str(subjectFeatures[i]))
           fout.write(",")
        i = i + 1
    for j in range(numberOfFeatures):
        if (featureNames[j] not in excludeList):
           fout.write(str(subjectFeatures[i]))
           fout.write(",")
        i = i + 1
    fout.write(str(letterCode))
    fout.write(",")
    fout.write(str(studyId))
    fout.write(",")
    fout.write(str(analysisId))
    fout.write("\n")

def writeDataJSON(featureNames,featureDict,excludeList,patientId,subjectFeatures,numberOfFeatures,studyId,letterCode,analysisId,fout):
    dobj = {}
    dobj["patient_id"] = patientId 
    dobj["analysis_id"] = analysisId 
    dobj["visit_id"] = "undefined"
    dobj["imaging_domain"] = "pathology"
    dobj["imaging_sequence"] = "H&E:tissue"

    nSubjectFeatures = len(subjectFeatures)    
    imaging_features = []
    i = 0 
    for j in range(numberOfFeatures):
        if (featureNames[j] not in excludeList):
           dobj2 = {}
           dobj2["feature_name"] = featureNames[j] + "_Q25"
           dobj2["value"] = subjectFeatures[i] 
           dobj2["feature_type"] = featureDict[featureNames[j]]
           imaging_features.append(dobj2)
        i = i + 1
    for j in range(numberOfFeatures):
        if (featureNames[j] not in excludeList):
           dobj2 = {}
           dobj2["feature_name"] = featureNames[j] + "_median"
           dobj2["value"] = subjectFeatures[i] 
           dobj2["feature_type"] = featureDict[featureNames[j]]
           imaging_features.append(dobj2)
        i = i + 1
    for j in range(numberOfFeatures):
        if (featureNames[j] not in excludeList):
           dobj2 = {}
           dobj2["feature_name"] = featureNames[j] + "_Q75"
           dobj2["value"] = subjectFeatures[i] 
           dobj2["feature_type"] = featureDict[featureNames[j]]
           imaging_features.append(dobj2)
        i = i + 1
    dobj["imaging_features"] = imaging_features 
    dobj["cancer_type"] = studyId
    dobj["tumor_type"]  = letterCode

    fout.write(str(dobj))
    fout.write("\n");

def computeAggregateFeatures(allContent,numberOfFeatures):
    nObjects = len(allContent)
    allFeatureMatrix = np.empty([nObjects, numberOfFeatures], dtype=float)

    for iObject in range(nObjects):
        featuresOfThisObject = allContent[iObject]
        featuresOfThisObject = featuresOfThisObject.split(',')
        featuresOfThisObject = featuresOfThisObject[:numberOfFeatures]
        featuresOfThisObject = [float(x) for x in featuresOfThisObject]

        allFeatureMatrix[iObject, :] = np.asarray(featuresOfThisObject)

    [featureQ25, featureMedian, featureQ75] = np.percentile(allFeatureMatrix, [25, 50, 75], axis=0)
    subjectFeatures = np.concatenate((featureQ25, featureMedian, featureQ75))
    return subjectFeatures

def printArgHelp():
    print 'mainAggregateFeatures.py -i <inputfile pattern> -d <dictionary file> [-e <exclude features file>] -p <patient id> -a <analysis id> -o <output file> -t <csv|json> -s <cancer type> -l <tumor type>'

def parseArguments(argv):
    dataDir = ''
    patientId = ''
    analysisId = ''
    outFile = ''
    cancerType = ''
    tumorType = ''
    dictFile = ''
    excludeFile=''
    outType = 'json'
    try:
		opts, args = getopt.getopt(argv,"hi:p:d:e:a:o:t:s:l:")
    except getopt.GetoptError:
       printArgHelp()
       sys.exit(2)
    if len(argv) == 0:
       printArgHelp()
       sys.exit(2)
    for opt, arg in opts:
       if opt == '-h':
          printArgHelp()
          sys.exit(2)
       elif opt == "-i": 
          dataDir = arg
       elif opt == "-d": 
          dictFile = arg
       elif opt == "-e": 
          excludeFile = arg
       elif opt == '-p':
          patientId = arg
       elif opt == '-a':
          analysisId = arg
       elif opt == '-o':
          outFile = arg
       elif opt == '-s':
          cancerType = arg
       elif opt == '-l':
          tumorType = arg
       elif opt == '-t':
          outType = arg

    if dataDir=='' or dictFile=='' or patientId=='' or analysisId=='' or cancerType=='' or outFile=='':
       printArgHelp()
       sys.exit(2)
    return (dataDir,patientId,analysisId,outFile,cancerType,tumorType,dictFile,excludeFile,outType)

def main(argv):
    # Parse input arguments
    [dataDir,patientId,analysisId,outFile,cancerType,tumorType,dictFile,excludeFile,outType] = parseArguments(argv)

    # Read the dictionary file
    featureDict = readDictionary(dictFile)

	# Read the list of features to be excluded
    excludeList = readExcludeFile(excludeFile)

    # Read the files 
    allContent, featureList, numberOfFeatures = readCSV(dataDir,featureDict)

	# Process object-level features. The last feature is Polygon and ignored
    nObjects = len(allContent) 
    subjectFeatures = computeAggregateFeatures(allContent,numberOfFeatures-1)

	# Write output to file
    fout = open(outFile,'w');
    if outType=='json':
       writeDataJSON(featureList,featureDict,excludeList,patientId,subjectFeatures,numberOfFeatures-1,cancerType,tumorType,analysisId,fout)
    else:
       writeDataCSV(featureList,excludeList,patientId,subjectFeatures,numberOfFeatures-1,cancerType,tumorType,analysisId,fout)
    fout.close()

if __name__ == '__main__':
	main(sys.argv[1:])
