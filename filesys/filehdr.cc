// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
// ****lab5 code begin****
#include "time.h"
// ****lab5 code end****

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space

// ****lab5 code begin****
    if(numSectors < NumDirect) {
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
	}
	else {
        for (int i = 0; i < NumDirect; i++)
            dataSectors[i] = freeMap->Find();
        int secondSectors = numSectors - NumDirect;
        int needNumSecondIndex = secondSectors/SecondDirect
            + ((secondSectors%SecondDirect) ? 1 : 0);
        printf("needed Num of Second Index is %d\n", needNumSecondIndex);
        int leftSectors = secondSectors;
        for (int i = 0; i < needNumSecondIndex; i++) {
            dataSectors[NumDirect+i] = freeMap->Find();
            int tempNumSectors = (leftSectors>SecondDirect)?SecondDirect:leftSectors;
            int* tempSectors = new int[tempNumSectors];
            for(int j=0;j<tempNumSectors;++j) {
                tempSectors[j] = freeMap->Find();
            }
            synchDisk->WriteSector(dataSectors[NumDirect+i], (char*)tempSectors);
            delete tempSectors;
            leftSectors -= SecondDirect;
        }
	}
    setCreateTime();
    setLastOpenTime();
    setUpdateTime();
    printf("create file time is %d\n", getCreateTime());
// ****lab5 code end****
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void
FileHeader::Deallocate(BitMap *freeMap)
{
// ****lab5 code begin****
    if(numSectors < NumDirect) {
        int i;
        for (i = 0; i < numSectors; i++)
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear(dataSectors[i]);
	}
	else {
        int i;
        for (i = 0; i < NumDirect; i++) {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear(dataSectors[i]);
        }
        int secondSectors = numSectors - NumDirect;
        int leftSectors = secondSectors;
        for(i=0;leftSectors>0;++i,leftSectors-=SecondDirect)
        {
            int tempNumSectors = (leftSectors>SecondDirect)?SecondDirect:leftSectors;
            char* tempRead = new char[SectorSize];
            synchDisk->ReadSector(dataSectors[NumDirect+i], tempRead);
            int* tempSectors = (int*)tempRead;
            for (int i = 0; i < tempNumSectors; i++) {
                ASSERT(freeMap->Test(tempSectors[i]));  // ought to be marked!
                freeMap->Clear(tempSectors[i]);
            }
            ASSERT(freeMap->Test((int) dataSectors[NumDirect+i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[NumDirect+i]);
        }
    }
// ****lab5 code end****
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
// ****lab5 code begin****
    int sectorOffset = offset/SectorSize;
    if(sectorOffset<NumDirect) {
        return dataSectors[sectorOffset];
    }
    else {
        int leftSectors=sectorOffset-NumDirect;
        int index=leftSectors/(SectorSize/sizeof(int));
        char* sectorTemp = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect+index], sectorTemp);
        int* sector = (int*)sectorTemp;
        int indexOffset=leftSectors-index*(SectorSize/sizeof(int));
        int returnValue=sector[indexOffset];
        delete sector;
        delete sectorTemp;
        return returnValue;
    }
//    return(dataSectors[offset / SectorSize]);
// ****lab5 code end****
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n");
    }
    delete [] data;
}
// ****lab5 code begin****
void
FileHeader::setCreateTime()
{
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    char* temp=asctime(timeinfo);
    strncpy(createTime, temp, DataForm);
}
void
FileHeader::setUpdateTime()
{
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    char* temp=asctime(timeinfo);
    strncpy(updateTime, temp, DataForm);
}
void
FileHeader::setLastOpenTime()
{
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    char* temp=asctime(timeinfo);
    strncpy(lastOpenTime, temp, DataForm);
}
// ****lab5 code end****
